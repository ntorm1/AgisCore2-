module;

#include "AgisDeclare.h"
#include "AgisMacros.h"

#include <ankerl/unordered_dense.h>

module StrategyModule;

import <string>;

import TradeModule;
import OrderModule;
import ExchangeModule;
import PortfolioModule;
import StrategyTracerModule;

namespace Agis
{

class StrategyPrivate
{
public:
	Portfolio& portfolio;
	std::string strategy_id;
	size_t strategy_index;
	size_t portfolio_index;
	size_t exchange_index;
	ankerl::unordered_dense::map<size_t, Trade const*> trades;

	StrategyPrivate(Portfolio& p, std::string id , size_t index)
		: strategy_id(id), strategy_index(index), portfolio(p)
	{}

	void place_order(std::unique_ptr<Order> order)
	{
		portfolio.place_order(std::move(order));
	}
};

//============================================================================
Strategy::Strategy(
	std::string id,
	size_t index,
	double cash,
	Exchange const& exchange,
	Portfolio& portfolio
	): 
	_exchange(exchange), _tracers(this, cash)
{
	_p = new StrategyPrivate(portfolio, id, index);
	_p->portfolio_index = portfolio.get_portfolio_index();
	_p->exchange_index = exchange.get_exchange_index();
}


//============================================================================
Portfolio*
Strategy::get_portfolio() const noexcept 
{
	return const_cast<Portfolio*>(&_p->portfolio); 
}


//============================================================================
void
Strategy::place_market_order(size_t asset_index, double units)
{
	auto order = std::make_unique<Order>(
		OrderType::MARKET_ORDER,
		asset_index,
		units,
		_p->strategy_index,
		_p->exchange_index,
		_p->portfolio_index
	);
	_p->place_order(std::move(order));
}


//============================================================================
size_t Strategy::get_strategy_index() const noexcept
{
	return _p->strategy_index;
}

//============================================================================
double
Strategy::get_cash() const noexcept
{
	return _tracers.get(CASH).value();
}


//============================================================================
void Strategy::reset() noexcept
{
	_p->trades.clear();
	_tracers.reset();
}


//============================================================================
void
Strategy::add_trade(Trade const* trade)
{
	_p->trades.insert({ trade->get_asset_index(), trade});
}


//============================================================================
void
Strategy::remove_trade(size_t asset_index)
{
	_p->trades.erase(asset_index);
}

}