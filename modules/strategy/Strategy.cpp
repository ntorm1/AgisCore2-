module;

#include "AgisDeclare.h"
#include "AgisMacros.h"

#include <Eigen/Dense>
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

std::atomic<size_t> Strategy::_strategy_counter(0);


class StrategyPrivate
{
public:
	Portfolio& portfolio;
	size_t strategy_index;
	size_t portfolio_index;
	size_t exchange_index;
	ankerl::unordered_dense::map<size_t, Trade const*> trades;

	StrategyPrivate(Portfolio& p, size_t index)
		:  strategy_index(index), portfolio(p)
	{}

	void place_order(std::unique_ptr<Order> order)
	{
		portfolio.place_order(std::move(order));
	}
};

//============================================================================
Strategy::Strategy(
	std::string id,
	double cash,
	Exchange const& exchange,
	Portfolio& portfolio
	): 
	_exchange(exchange), _tracers(this, cash)
{
	_p = new StrategyPrivate(portfolio, _strategy_counter++);
	_p->portfolio_index = portfolio.get_portfolio_index();
	_p->exchange_index = exchange.get_exchange_index();
	_strategy_id = id;
}


//============================================================================
Portfolio*
Strategy::get_portfolio_mut() const noexcept
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
		this,
		_p->strategy_index,
		_p->exchange_index,
		_p->portfolio_index
	);
	_p->place_order(std::move(order));
}


//============================================================================
std::expected<bool, AgisException>
Strategy::set_allocation(
	Eigen::VectorXd& allocations,
	double epsilon,
	bool clear_missing) noexcept
{
	double nlv = this->get_nlv();
	size_t i = 0;
	size_t exchange_offset = _exchange.get_index_offset();
	for (auto& allocation : allocations)
	{
		size_t asset_index = i + exchange_offset;
		if(!allocation) continue;
		AGIS_OPTIONAL_MOVE(market_price, _exchange.get_market_price(asset_index));
		double size = (nlv * allocation) / market_price;

		// check min size 
		if(abs(size) < ORDER_EPSILON) continue;

		auto trade_opt = this->get_trade_mut(asset_index);
		if (trade_opt)
		{
			auto& trade = trade_opt.value();
			if (clear_missing) trade->set_strategy_alloc_touch(true);
			auto exsisting_units = trade->get_units();
			size -= exsisting_units;

			// test to see if the new order is within epsilon of the exsisting order
			double offset = abs(size) / abs(exsisting_units);
			if (epsilon > 0.0 && offset < epsilon) { continue; }
			// if epsilon is less than 0, only place new order if the new order is reversing the 
			// direction of the trade
			else if (epsilon < 0.0) {
				if (size * exsisting_units > 0) continue;
				if (size * exsisting_units < 0 && abs(size) < abs(exsisting_units)) continue;
			}
		}
		this->place_market_order(asset_index, size);
	}

	// if clear missing is true, then clear any trades that are not in the exchange view
	if (clear_missing && _p->trades.size())
	{
		for (auto& trade : _p->trades)
		{
			if(!trade.second->is_strategy_alloc_touch())
			{
				this->place_market_order(trade.first, -1 * trade.second->get_units());
			}
			trade.second->set_strategy_alloc_touch(false);
		}
	
	}

	return true;
}


//============================================================================
std::optional<Trade const*>
Strategy::get_trade(size_t asset_index) const noexcept
{
	auto it = _p->trades.find(asset_index);
	if(it == _p->trades.end()) return std::nullopt;
	return it->second;
}


//============================================================================
size_t Strategy::get_strategy_index() const noexcept
{
	return _p->strategy_index;
}


//============================================================================
std::optional<size_t>
Strategy::get_asset_index(std::string const& asset_id)
{
	return _exchange.get_asset_index(asset_id);
}


//============================================================================
std::optional<Trade*> Strategy::get_trade_mut(size_t asset_index) const noexcept
{
	auto opt = this->get_trade(asset_index);
	if (!opt) return std::nullopt;
	return const_cast<Trade*>(opt.value());
}


//============================================================================
double
Strategy::get_cash() const noexcept
{
	return _tracers.get(CASH).value();
}


//============================================================================
double
Strategy::get_nlv() const noexcept
{
	return _tracers.get(NLV).value();
}


//============================================================================
size_t Strategy::get_strategy_index()
{
	return _p->strategy_index;
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