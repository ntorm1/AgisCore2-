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
	_tracers.nlv_add_assign(_tracers.get(Tracer::CASH).value());

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
	auto o = order.get();
	if (!_exchange) return;
	auto res = _exchange->place_order(std::move(order));
	if(res)
	{
		auto order_state = order->get_order_state();
		this->process_order(std::move(order));
	}
}


//============================================================================
void
Portfolio::process_filled_order(Order* order)
{
	auto lock = std::lock_guard(_mutex);

	auto position_opt = get_position(order->get_asset_index());
	if (!position_opt)
	{
		this->open_position(order);
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
	}
	// adjust cash levels required by the order. Intial call on the base portfolio will adjust
	// cash levels all the way up the portfolio tree
	auto cash_adjustment = order->get_units() * order->get_fill_price();
	if (order->get_portfolio_index() == _portfolio_index)
	{
		_tracers.cash_add_assign(-cash_adjustment);
	}
	// propogate the order up the portfolio tree and drop the lock on this portfolio
	lock.~lock_guard();
	if (_parent_portfolio)
	{
		_parent_portfolio.value()->process_filled_order(order);
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
	for (auto& trade : trades)
	{
		_trade_history.push_back(std::move(trade.second));
	}
	_tracers.unrealized_pnl_add_assign(-1*position->get_unrealized_pnl());
	auto it = _positions.find(order->get_asset_index());
	_positions.erase(order->get_asset_index());
	_position_history.push_back(std::move(it->second));
}


//============================================================================
void
Portfolio::adjust_position(Order* order, Position* position) noexcept
{
	auto strategy = _strategies.at(order->get_strategy_index()).get();
	position->adjust(strategy, order, _trade_history);
}


//============================================================================
void
Portfolio::open_position(Order const* order) noexcept
{
	auto& strategy = _strategies.at(order->get_strategy_index());
	_positions.emplace(order->get_asset_index(), std::make_unique<Position>(strategy.get(), order));
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
Portfolio::get_position(size_t asset_index) const noexcept
{
	auto it = _positions.find(asset_index);
	if (it == _positions.end())
	{
		return std::nullopt;
	}
	return it->second.get();
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