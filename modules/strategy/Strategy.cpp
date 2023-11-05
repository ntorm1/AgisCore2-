module;

#include "AgisDeclare.h"
#include "AgisMacros.h"

#include <ankerl/unordered_dense.h>

module StrategyModule;

import <string>;

import TradeModule;
import StrategyTracerModule;

namespace Agis
{

struct StrategyPrivate
{
	std::string id;
	size_t index;
	ankerl::unordered_dense::map<size_t, Trade const*> trades;

	StrategyPrivate(std::string id , size_t index)
		: id(id), index(index)
	{}
};

//============================================================================
Strategy::Strategy(
	std::string id,
	size_t index,
	double cash,
	Exchange const& exchange,
	Portfolio const& portfolio
	): 
	_exchange(exchange), _portfolio(portfolio), _tracers(this, cash)
{
	_p = new StrategyPrivate(id, index);
}


//============================================================================
void
Strategy::place_market_order(size_t asset_index, double units)
{
	
}


//============================================================================
size_t Strategy::get_strategy_index() const noexcept
{
	return _p->index;
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