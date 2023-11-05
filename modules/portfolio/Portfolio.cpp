module;

#include <tbb/task_group.h>
#include "AgisDeclare.h"
#include "AgisMacros.h"

module PortfolioModule;

import OrderModule;
import PositionModule;
import ExchangeModule;
import ExchangeMapModule;
import TradeModule;
import StrategyModule;

import <string>;


namespace Agis
{


//============================================================================
Portfolio::Portfolio(
	tbb::task_group& _task_group,
	std::string id,
	size_t index,
	std::optional<Exchange*>	exchange,
	std::optional<Portfolio*>	parent_portfolio,	
	double cash
) : _task_group(_task_group), _tracers(this, cash)
{
	_portfolio_id = id;
	_portfolio_index = index;
	if (parent_portfolio)
	{
		_parent_portfolio = parent_portfolio.value();
	}
	if (exchange)
	{
		_exchange = exchange.value();
		_exchange->register_portfolio(this);
	}
}


//============================================================================
Portfolio::~Portfolio()
{
	for (auto& trade : _trade_history)
	{
		delete trade;
	}
}


//============================================================================
std::optional<Position const*>
Portfolio::get_position(std::string const& asset_id) const noexcept
{
	// get the asset index from the exchange map or exchange
	std::optional<size_t> asset_index;
	if (_exchange_map) asset_index = _exchange_map.value()->get_asset_index(asset_id);
	else asset_index = _exchange->get_asset_index(asset_id);

	if(!asset_index) return std::nullopt;

	// find the position in the portfolio
	auto it = _positions.find(asset_index.value());
	if (it == _positions.end())
	{
		return std::nullopt;
	}
	return it->second.get();
}


//============================================================================
void Portfolio::reset()
{
	_tracers.reset();
	_trade_history.clear();
	_order_history.clear();
	_position_history.clear();
	_positions.clear();

	for (auto& [index, strategy] : _strategies)
	{
		strategy->reset();
	}
	for (auto& [index, portfolio] : _child_portfolios)
	{
		portfolio->reset();
	}
}


//============================================================================
void
Portfolio::zero_out()
{
	_tracers.zero_out_tracers();
	for (auto& [index, strategy] : _strategies)
	{
		strategy->_tracers.zero_out_tracers();
	}
	for (auto& [index, portfolio] : _child_portfolios)
	{
		portfolio->zero_out();
	}
}


//============================================================================
std::expected<bool, AgisException>
Portfolio::evaluate(bool on_close, bool is_reprice)
{
	if(!_exchange_map) return std::unexpected(AgisException("master portfolio has no exchange map"));
	// when evaluating the portfolio we need to zero out strategy and child portfolio levels so they can 
	// be recalculated using their respective trades
	this->zero_out();

	// evalute all open positions and their respective trades
	std::vector<std::unique_ptr<Order>> filled_orders;
	for (auto it = this->_positions.begin(); it != _positions.end(); ++it)
	{
		// evaluate the indivual positions and allow for any orders that are generated
		// as a result of the new valuation
		auto& position = it->second;
		position->evaluate(on_close, is_reprice);
		if (is_reprice) continue;

		// force close any positions whos asset is no longer available
		if (position->is_last_row())
		{
			auto order = position->generate_position_inverse_order();
			order->set_order_state(OrderState::CHEAT);
			order->set_force_close(true);
			auto res = _exchange_map.value()->force_place_order(order.get(), on_close);
			if(!res) return std::unexpected(res.error());
			filled_orders.push_back(std::move(order));
		}
	}

	for (auto& order : filled_orders)
	{
		this->process_order(std::move(order));
	}

	if (is_reprice) return true;
	return _tracers.evaluate();
}


//============================================================================
std::expected<bool, AgisException>
Portfolio::step()
{
	if (!_step_call) return true;
	for (auto& strategy_pair : _strategies) {
		auto strategy = strategy_pair.second.get();
		_task_group.run([strategy]() {
			if (strategy->has_exception()) {
				return;
			}
			auto res = strategy->step();
			if (!res) {
				strategy->set_exception(std::move(res.error()));
			}
		});
	}
	_step_call = false;
	return true;
}


//============================================================================
std::expected<Portfolio*, AgisException> Portfolio::add_child_portfolio(
	std::string id,
	size_t index,
	std::optional<Exchange*> exchange,
	double cash)
{
	// create the portfolio
	if (_child_portfolios.find(index) != _child_portfolios.end())
	{
		return std::unexpected<AgisException>("child portfolio index exists");
	}
	auto portfolio = std::make_unique<Portfolio>(_task_group, id, index, exchange, this, cash);
	auto p = portfolio.get();
	_child_portfolios[index] = std::move(portfolio);

	// adjust cash levels all the way up the tree
	_tracers.starting_cash_add_assign(cash);
	return p;
}


//============================================================================
std::expected<bool, AgisException>
Portfolio::add_strategy(std::unique_ptr<Strategy> strategy)
{
	if(!_exchange)
	{
		return std::unexpected(AgisException("Portfolio has no exchange"));
	}
	_tracers.starting_cash_add_assign(strategy->get_cash());
	_strategies.emplace(
		strategy->get_strategy_index(),
		std::move(strategy)
	);
	return true;
}


//============================================================================
void
Portfolio::place_order(std::unique_ptr<Order> order) noexcept
{
	if (!_exchange) return;
	auto res = _exchange->place_order(std::move(order));
	if(res)
	{
		auto o = std::move(res.value());
		this->process_order(std::move(o));
	}
}


//============================================================================
void
Portfolio::process_filled_order(Order* order)
{
	std::unique_lock<std::shared_mutex> lock(_mutex);

	auto position_opt = get_position_mut(order->get_asset_index());
	if (!position_opt)
	{
		this->open_position(order);
		if (_parent_portfolio) 
		{
			auto strategy_index = order->get_strategy_index();
			auto trade = this->get_trade_mut(order->get_asset_index(), strategy_index).value();
			_parent_portfolio.value()->open_position(trade);
		}	
	}
	else
	{
		auto position = position_opt.value();
		auto order_units = order->get_units();
		auto position_units = position->get_units();
		// if the new order units plut existing position units is not zero then the order
		// is an adjustment of the current position
		if (abs(order_units + order_units) > UNIT_EPSILON)
		{
			this->adjust_position(order, position);
		}
		// if the order is force closed, set at the portfolio level, then force close the postion
		// even if the position consits of multipule trades.
		else if (order->_force_close)
		{
			this->close_position(order, position);
		}
		// if the positions new units are 0 but there is an existing trade with a different strategy id
		// then allow for the position to be adjusted and note close by this order
		else if (
			!position->trade_exists(order->get_strategy_index()) 
			||
			position->get_trades().size() > 1
			)
		{
			this->adjust_position(order, position);
		}
		else this->close_position(order, position);

		lock.unlock();
		if (_parent_portfolio)
		{
			process_filled_order(order);
		}
	}
	// adjust cash levels required by the order. Intial call on the base portfolio will adjust
	// cash levels all the way up the portfolio tree
	auto cash_adjustment = order->get_units() * order->get_fill_price();
	if (order->get_portfolio_index() == _portfolio_index)
	{
		_tracers.cash_add_assign(-cash_adjustment);
		order->get_strategy_mut()->_tracers.cash_add_assign(-cash_adjustment);
	}
}


//============================================================================
void Portfolio::process_order(std::unique_ptr<Order> order)
{
	switch (order->get_order_state())
	{
	case OrderState::FILLED:
		this->process_filled_order(order.get());
		break;
	default:
		break;
	}
	auto lock = std::lock_guard(_mutex);
	_order_history.push_back(std::move(order));
}


//============================================================================
void
Portfolio::close_position(Order const* order, Position* position) noexcept
{
	position->close(order);
	auto& trades = position->get_trades();
	for (auto& [strategy_index, trade] : trades)
	{
		if(trade->get_portfolio_index() == _portfolio_index)
		_trade_history.push_back(trade);
	}
	
	_tracers.unrealized_pnl_add_assign(-1*position->get_unrealized_pnl());
	auto it = _positions.find(order->get_asset_index());
	_positions.erase(order->get_asset_index());
	_position_history.push_back(std::move(it->second));
}


//============================================================================
void Portfolio::close_trade(size_t asset_index, size_t strategy_index) noexcept
{
	auto position = get_position_mut(asset_index).value();
	position->close_trade(strategy_index);
	while (_parent_portfolio) {
		_parent_portfolio.value()->close_trade(asset_index, strategy_index);
	}
}


//============================================================================
void Portfolio::insert_trade(Trade* trade) noexcept
{
	auto position = get_position_mut(trade->get_asset_index()).value();
	position->adjust(trade);
	while (_parent_portfolio) {
		_parent_portfolio.value()->insert_trade(trade);
	}
}


//============================================================================
void
Portfolio::adjust_position(Order* order, Position* position) noexcept
{
	auto history_size = _trade_history.size();
	auto init_trade_opt = position->get_trade_mut(order->get_strategy_index());
	position->adjust(order->get_strategy_mut(), order, _trade_history);
	
	if (!_parent_portfolio) return;

	// if the trade was closed then remove it from the trade history and propogate close up
	if (_trade_history.size() > history_size)
	{
		auto trade = _trade_history.back();
		_parent_portfolio.value()->close_trade(trade->get_asset_index(), trade->get_strategy_index());
		return;
	}
	// if trade is new then insert it up the tree
	auto trade = position->get_trade_mut(order->get_strategy_index()).value();
	if (!init_trade_opt)
	{
		_parent_portfolio.value()->insert_trade(trade);
	}
	// if trade was reversed then there is a new trade pointer 
	else if (init_trade_opt.value() != trade)
	{
		auto asset_index = trade->get_asset_index();
		auto strategy_index = trade->get_strategy_index();
		_parent_portfolio.value()->close_trade(asset_index, strategy_index);
		_parent_portfolio.value()->insert_trade(trade);
	}
}


//============================================================================
void
Portfolio::open_position(Order const* order) noexcept
{
	_positions.emplace(
		order->get_asset_index(),
		std::make_unique<Position>(order->get_strategy_mut(), order)
	);
}


//============================================================================
void
Portfolio::open_position(Trade* trade) noexcept
{
	_positions.emplace(
		trade->get_asset_index(),
		std::make_unique<Position>(trade->get_strategy_mut(), trade)
	);
	while (_parent_portfolio) {
		_parent_portfolio.value()->open_position(trade);
	}
}


//============================================================================
std::optional<Exchange const*>
Portfolio::get_exchange() const noexcept
{
	if(!_exchange)
	{
		return std::nullopt;
	}
	return _exchange;
}

//============================================================================
std::optional<Position*>
Portfolio::get_position_mut(size_t asset_index) const noexcept
{
	auto it = _positions.find(asset_index);
	if (it == _positions.end())
	{
		return std::nullopt;
	}
	return it->second.get();
}


//============================================================================
std::optional<Trade*> Portfolio::get_trade_mut(size_t asset_index, size_t strategy_index) const noexcept
{
	auto position = get_position_mut(asset_index);
	if (!position) return std::nullopt;
	return position.value()->get_trade_mut(strategy_index);
}


//============================================================================
bool
Portfolio::position_exists(size_t index) const noexcept
{
	return _positions.find(index) != _positions.end();
}


//============================================================================
bool
Portfolio::trade_exists(size_t asset_index, size_t strategy_index) const noexcept
{
	auto it = _positions.find(asset_index);
	if (it == _positions.end())
	{
		return false;
	}
	return it->second->trade_exists(strategy_index);
}


//============================================================================
double
Portfolio::get_cash() const noexcept
{
	return _tracers.get(Tracer::CASH).value();
}


//============================================================================
double Portfolio::get_nlv() const noexcept
{
	return _tracers.get(Tracer::NLV).value();
}

}