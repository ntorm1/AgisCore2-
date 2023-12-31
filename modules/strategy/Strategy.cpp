module;

#include "AgisDeclare.h"
#include "AgisMacros.h"

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <Eigen/Dense>
#include <ankerl/unordered_dense.h>

module StrategyModule;

import <string>;

import TradeModule;
import OrderModule;
import ExchangeModule;
import PortfolioModule;
import StrategyTracerModule;
import AgisXPool;

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
	ObjectPool<Order> order_pool;
	StrategyPrivate(Portfolio& p, size_t index)
		: strategy_index(index), portfolio(p), order_pool(1000)
	{
	}

	inline void place_order(std::unique_ptr<Order> order) const noexcept
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
	_exchange(exchange), _tracers(this, cash, exchange.get_assets().size(), exchange.get_index_offset())
{
	_p = new StrategyPrivate(portfolio, _strategy_counter++);
	_p->portfolio_index = portfolio.get_portfolio_index();
	_p->exchange_index = exchange.get_exchange_index();
	_strategy_id = id;
}


//============================================================================
void
Strategy::set_exception(AgisException&& exception) noexcept
{
	_exception = exception; 
	_is_disabled = true;
}

//============================================================================
size_t Strategy::get_asset_count_limit() const noexcept
{
	return _exchange.get_assets().size();
}


//============================================================================
size_t Strategy::get_exchange_offset()
const noexcept
{
	return _exchange.get_index_offset();
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
	auto order = _p->order_pool.pop_unique(
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
std::expected<bool, AgisException>
Strategy::set_allocation(Eigen::VectorXd& nlvs) noexcept
{
	size_t exchange_offset = _exchange.get_index_offset();
	double nlv = this->get_nlv();
	size_t i = 0;
	for (auto allocation : nlvs)
	{
		size_t asset_index = i + exchange_offset;
		if (!allocation) 
		{
			i++;
			continue;
		};
#ifdef _DEBUG
		AGIS_OPTIONAL_MOVE(market_price, _exchange.get_market_price(asset_index));
#else
		auto market_price = *_exchange.get_market_price(asset_index);
#endif
		double size = (nlv * allocation) / market_price;
		if (abs(size) < ORDER_EPSILON) 
		{
			i++;
			continue;
		};
		this->place_market_order(asset_index, size);
		i++;

	}
	return true;
}


//============================================================================
void
Strategy::serialize_base(rapidjson::Document& j, rapidjson::Document::AllocatorType& allocator) const noexcept
{
	rapidjson::Value v_id(_strategy_id.c_str(), allocator);
	j.AddMember("strategy_id", v_id.Move(), allocator);

	rapidjson::Value v_exchange_id(_exchange.get_exchange_id().c_str(), allocator);
	j.AddMember("exchange_id", v_exchange_id.Move(), allocator);

	rapidjson::Value v_portfolio_id(_p->portfolio.get_portfolio_id().c_str(), allocator);
	j.AddMember("portfolio_id", v_portfolio_id.Move(), allocator);

	j.AddMember("starting_cash", _tracers.starting_cash.load(), allocator);
}


//============================================================================
std::string const&
Strategy::get_exchange_id() const noexcept
{
	return _exchange.get_exchange_id();
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
std::string const& Strategy::get_portfolio_id() const noexcept
{
	return _p->portfolio.get_portfolio_id();
}


//============================================================================
void Strategy::reset() noexcept
{
	_p->trades.clear();
	_tracers.reset();	
	_exception = std::nullopt;
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
	_tracers.zero_allocation(asset_index);
	_p->trades.erase(asset_index);
}

Eigen::VectorXd const& Strategy::get_weights() const noexcept
{
	return _tracers.get_weights();
}

}