module;
#include <cassert>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_vector.h>
#include <tbb/task_group.h>
#include <boost/container/flat_map.hpp>
#include <condition_variable>
#include "AgisDeclare.h"
#include "AgisMacros.h"

module PortfolioModule;

import OrderModule;
import PositionModule;
import ExchangeModule;
import ExchangeMapModule;
import TradeModule;
import StrategyModule;
import AgisXPool;

import <string>;




namespace Agis
{

using TbbAccessor = tbb::concurrent_hash_map<size_t, Position*>::accessor;


//============================================================================
class PortfolioPrivate
{
public:
	ObjectPool<Position> position_pool;
	std::unique_ptr<std::mutex[]>  _asset_mutex_map;
	std::unique_ptr<std::condition_variable_any[]> _asset_cv_map;
	tbb::concurrent_vector<Trade*>					trade_history;
	tbb::concurrent_vector<Order*>					order_history;
	size_t exchange_offset = 0;
	PortfolioPrivate(size_t init_position_pool_size) : position_pool(init_position_pool_size) {}
	~PortfolioPrivate() {}
};


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
	_p = new PortfolioPrivate(1000);
	_portfolio_id = id;
	_portfolio_index = index;
	if (parent_portfolio)
	{
		_parent_portfolio = parent_portfolio.value();
	}
	else
	{
		assert(id == "master");
	}
	if (exchange)
	{
		_p->exchange_offset = exchange.value()->get_index_offset();
		_exchange = exchange.value();
		_exchange.value()->register_portfolio(this);
		build_mutex_map();
	}
}


//============================================================================
Portfolio::~Portfolio()
{
	free_object_vector<Trade>(_p->trade_history);
	free_object_vector<Order>(_p->order_history);
}


//============================================================================
void
Portfolio::build_mutex_map() noexcept
{
	size_t new_size;
	// init the asset mutex map with single thread to prevent bad allocs
	if (_exchange)
	{
		new_size =  _exchange.value()->get_assets().size();
	}
	else
	{
		auto const& assets = (*_exchange_map)->get_assets();
		new_size = assets.size();
	}
	_p->_asset_mutex_map.reset(new std::mutex[new_size]);
	_p->_asset_cv_map.reset(new std::condition_variable_any[new_size]);
}

//============================================================================
std::mutex&
Portfolio::get_asset_mutex(size_t asset_index) const noexcept
{
	asset_index -= _p->exchange_offset;
	auto& asset_mutex = _p->_asset_mutex_map[asset_index];
;	std::unique_lock<std::mutex> lock(asset_mutex);

	// Wait until the mutex is available
	_p->_asset_cv_map[asset_index].wait(lock, [&] { return !asset_mutex.try_lock(); });

	// Lock is now acquired
	return asset_mutex;
}


//============================================================================
std::unique_lock<std::shared_mutex>
Portfolio::__aquire_write_lock() const noexcept
{
	return std::unique_lock<std::shared_mutex>(_mutex);
}


//============================================================================
std::shared_lock<std::shared_mutex> Portfolio::__aquire_read_lock() const noexcept
{
	return std::shared_lock<std::shared_mutex>(_mutex);
}


//============================================================================
std::expected<bool, AgisException>
Portfolio::remove_strategy(Strategy& strategy)
{
	if (
		!_strategies.count(strategy.get_strategy_index()) ||
		strategy.get_portfolio_mut() != this)
	{
		return std::unexpected(AgisException("strategy does not belong to portfolio"));
	}
	_strategies.erase(strategy.get_strategy_index());
	return true;
}


//============================================================================
std::optional<Position const*>
Portfolio::get_position(std::string const& asset_id) const noexcept
{
	// get the asset index from the exchange map or exchange
	std::optional<size_t> asset_index;
	if (_exchange_map) asset_index = _exchange_map.value()->get_asset_index(asset_id);
	else asset_index = _exchange.value()->get_asset_index(asset_id);

	if(!asset_index) return std::nullopt;

	// find the position in the portfolio
	if(!_positions.count(asset_index.value()))
	{
		return std::nullopt;
	}
	tbb::concurrent_hash_map<size_t, Position*>::accessor accessor;
	if (_positions.find(accessor, asset_index.value())) {
		return accessor->second;
	}
	return std::nullopt;
}


//============================================================================
void
Portfolio::build(size_t n)
{
	_tracers.build(n);
	for (auto& [index, strategy] : _strategies)
	{
		strategy->build(n);
	}
	for (auto& [index, portfolio] : _child_portfolios)
	{
		portfolio->build(n);
	}
}


//============================================================================
template<typename T>
void Portfolio::free_object_vector(tbb::concurrent_vector<T*>& objects)
{
	for (auto& object : objects)
	{
		assert(object);
		auto parent_portfolio = object->get_parent_portfolio_mut();
		assert(parent_portfolio);
		if (parent_portfolio != this) continue;
		delete object;
	}
	objects.clear();
}


//============================================================================
void Portfolio::reset()
{
	_tracers.reset();
	free_object_vector<Trade>(_p->trade_history);
	free_object_vector<Order>(_p->order_history);
	_positions.clear();
	_p->position_pool.reset();

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
			auto& trades = position->get_trades();
			for (auto& [strategy_index, trade] : trades)
			{
				auto order = trade->generate_trade_inverse();
				order->set_order_state(OrderState::CHEAT);
				order->set_force_close(true);
				auto res = _exchange_map.value()->force_place_order(order.get(), on_close);
				if (!res) return std::unexpected(res.error());
				filled_orders.push_back(std::move(order));
			}
		}
	}

	for (auto& order : filled_orders)
	{
		auto portfolio = order->get_parent_portfolio_mut();
		portfolio->process_order(std::move(order));
	}
	return _tracers.evaluate(is_reprice);
}


//============================================================================
std::expected<bool, AgisException>
Portfolio::step()
{
	if (_step_call) {
		for (auto& strategy_pair : _strategies) {
			auto strategy = strategy_pair.second.get();
			_task_group.run([strategy]() {
				if (strategy->has_exception() || strategy->is_disabled()) {
					return;
				}
				auto res = strategy->step();
				if (!res) {
					strategy->set_exception(std::move(res.error()));
				}
				});
		}
	}
	for (auto& [index, child_portfolio] : _child_portfolios) {
		AGIS_ASSIGN_OR_RETURN(res, child_portfolio->step());
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
	order->set_parent_portfolio(this);
	if (!_exchange) return;
	auto res = _exchange.value()->place_order(std::move(order));
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
	auto asset_index = order->get_asset_index();
	std::unique_lock<std::mutex> lock(_p->_asset_mutex_map[asset_index]);

	auto position_opt = get_position_mut(asset_index);
	if (!position_opt)
	{
		this->open_position(order);
		if (_parent_portfolio) 
		{
			auto strategy_index = order->get_strategy_index();
			auto trade = this->get_trade_mut(asset_index, strategy_index).value();
			_parent_portfolio.value()->open_position(trade);
		}	
	}
	else
	{
		auto position = position_opt.value();
		auto order_units = order->get_units();
		auto position_units = position->get_units();
		auto combined_units = abs(order_units + position_units);
		// if the new order units plut existing position units is not zero then the order
		// is an adjustment of the current position
		if (combined_units > UNIT_EPSILON)
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
	lock.unlock();

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
void
Portfolio::remember_order(Order* order) noexcept
{
	if (_tracers.has(Tracer::ORDERS))
	{
		_p->order_history.push_back(order);
		if (_parent_portfolio)
		{
			_parent_portfolio.value()->remember_order(order);
		}
	}
	else
	{
		delete order;
	}	
}

//============================================================================
void Portfolio::process_order(UniquePtr<Order> order)
{
	switch (order->get_order_state())
	{
	case OrderState::FILLED:
		this->process_filled_order(order.get());
		break;
	default:
		break;
	}
	// release unique_ptr ownership in order to share order up portfolio tree. On reset or on Portfolio
	// destruction, the order will be deleted by it's respective parent portfolio (whoever placed it).
	this->remember_order(order.release());
}


//============================================================================
void
Portfolio::close_position(Order const* order, Position* position) noexcept
{
	position->close(order);
	auto& trades = position->get_trades();
	for (auto& [strategy_index, trade] : trades)
	{
		_p->trade_history.push_back(trade);
		if (_parent_portfolio)
		{
			auto asset_index = trade->get_asset_index();
			auto& parent_mutex = _parent_portfolio.value()->get_asset_mutex(asset_index);
			std::unique_lock<std::mutex> lock(parent_mutex);
			_parent_portfolio.value()->close_trade(
				asset_index,
				trade->get_strategy_index()
			);
		}
	}
	auto asset_index = order->get_asset_index();
	_tracers.unrealized_pnl_add_assign(-1*position->get_unrealized_pnl());
	TbbAccessor accessor;
	_positions.find(accessor, asset_index);
	_positions.erase(accessor);
}


//============================================================================
void
Portfolio::close_trade(size_t asset_index, size_t strategy_index) noexcept
{
	auto position_opt = get_position_mut(asset_index);
	assert(position_opt);
	auto position = position_opt.value();
	position->close_trade(strategy_index);
	if (position->get_trades().size() == 0)
	{
		TbbAccessor accessor;
		_positions.find(accessor, asset_index);
		_positions.erase(accessor);
	}
	if (_parent_portfolio) 
	{
		auto& parent_mutex = _parent_portfolio.value()->get_asset_mutex(asset_index);
		parent_mutex.lock();
		_parent_portfolio.value()->close_trade(asset_index, strategy_index);
		parent_mutex.unlock();
	}
}


//============================================================================
void Portfolio::insert_trade(Trade* trade) noexcept
{
	auto asset_index = trade->get_asset_index();
	auto position = get_position_mut(asset_index).value();
	position->adjust(trade);
	if (_parent_portfolio) 
	{
		// try to obtain a write lock on the asset mutex
		auto& parent_mutex = _parent_portfolio.value()->get_asset_mutex(asset_index);
		parent_mutex.lock();
		_parent_portfolio.value()->insert_trade(trade);
		parent_mutex.unlock();
	}
}


//============================================================================
void
Portfolio::adjust_position(Order* order, Position* position) noexcept
{
	auto init_trade_opt = position->get_trade_mut(order->get_strategy_index());
	auto closed_trade_opt = position->adjust(order->get_strategy_mut(), order);
	
	if (!_parent_portfolio) return;

	// if the trade was closed then remove it from the trade history and propogate close up
	if (closed_trade_opt)
	{
		auto trade = closed_trade_opt.value();
		auto asset_index = trade->get_asset_index();
		auto& parent_mutex = _parent_portfolio.value()->get_asset_mutex(asset_index);
		std::unique_lock<std::mutex> lock(parent_mutex);
		_parent_portfolio.value()->close_trade(asset_index, trade->get_strategy_index());
		return;
	}
	// if trade is new then insert it up the tree
	auto trade = position->get_trade_mut(order->get_strategy_index()).value();
	auto asset_index = trade->get_asset_index();
	auto& parent_mutex = _parent_portfolio.value()->get_asset_mutex(asset_index);
	parent_mutex.lock();
	if (!init_trade_opt)
	{
		_parent_portfolio.value()->insert_trade(trade);
	}
	// if trade was reversed then there is a new trade pointer 
	else if (init_trade_opt.value() != trade)
	{
		auto strategy_index = trade->get_strategy_index();
		_parent_portfolio.value()->close_trade(asset_index, strategy_index);
		_parent_portfolio.value()->insert_trade(trade);
	}
	parent_mutex.unlock();
}


//============================================================================
void
Portfolio::open_position(Order const* order) noexcept
{
	auto asset_index = order->get_asset_index();
	Position* position = _p->position_pool.get(order->get_strategy_mut(), order);
	this->set_child_portfolio_position_parents(position);
	tbb::concurrent_hash_map<size_t, Position*>::accessor accessor;
	_positions.insert(accessor, asset_index);
	accessor->second = position;
}


//============================================================================
void
Portfolio::open_position(Trade* trade) noexcept
{
	// open position called with trade occurs when a child portfolio propogates a new 
	// open position up, therefore need to aquire a write lock on the asset mutex
	auto asset_index = trade->get_asset_index();
	auto& mutex = get_asset_mutex(asset_index);
	auto unique_lock = std::unique_lock(mutex);


	if (position_exists(asset_index))
	{
		this->insert_trade(trade);
	}
	else
	{
		Position* position = _p->position_pool.get(trade->get_strategy_mut(), trade);
		this->set_child_portfolio_position_parents(position);
		tbb::concurrent_hash_map<size_t, Position*>::accessor accessor;
		_positions.insert(accessor, asset_index);
		accessor->second = position;
	}
	while (_parent_portfolio) 
	{
		_parent_portfolio.value()->open_position(trade);
	}
}


//============================================================================
size_t
Portfolio::get_asset_count_limit() const noexcept
{
	if (_exchange)
		return _exchange.value()->get_assets().size();
	else
		return _exchange_map.value()->get_assets().size();
}


//============================================================================
void Portfolio::set_child_portfolio_position_parents(Position* p) noexcept
{
	for (auto& [index, portfolio] : _child_portfolios)
	{
		auto position = portfolio->get_position_mut(p->get_asset_index());
		if (position)
		{
			position.value()->set_parent_position(p);
		}
	}
}


//============================================================================
std::optional<Exchange const*>
Portfolio::get_exchange() const noexcept
{
	return _exchange;
}




//============================================================================
std::optional<Position*>
Portfolio::get_parent_position(size_t asset_index) const noexcept
{
	if (!_parent_portfolio) return std::nullopt;
	return _parent_portfolio.value()->get_position_mut(asset_index);
}


//============================================================================
std::optional<Position*>
Portfolio::get_position_mut(size_t asset_index) const noexcept
{
	tbb::concurrent_hash_map<size_t, Position*>::accessor accessor;
	if (_positions.find(accessor, asset_index)) {
		return accessor->second;
	}
	return std::nullopt;
}


//============================================================================
std::optional<Trade*> Portfolio::get_trade_mut(size_t asset_index, size_t strategy_index) const noexcept
{
	auto position = get_position_mut(asset_index);
	if (!position) 
	{
		return std::nullopt;
	}
	return position.value()->get_trade_mut(strategy_index);
}


//============================================================================
bool
Portfolio::position_exists(size_t index) const noexcept
{
	tbb::concurrent_hash_map<size_t, Position*>::accessor accessor;
	if (_positions.find(accessor, index)) {
		return true;
	}
	return false;
}


//============================================================================
bool
Portfolio::trade_exists(size_t asset_index, size_t strategy_index) const noexcept
{
	tbb::concurrent_hash_map<size_t, Position*>::accessor accessor;
	if (_positions.find(accessor, asset_index)) {
		return accessor->second->trade_exists(strategy_index);
	}
	return false;
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


//============================================================================
tbb::concurrent_vector<Order*> const&
Portfolio::order_history() const noexcept
{
	return _p->order_history;
}

}