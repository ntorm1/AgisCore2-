module;

#include "AgisMacros.h"
#include "AgisDeclare.h"
#include <tbb/task_group.h>
#include <ankerl/unordered_dense.h>

module HydraModule;

import PortfolioModule;
import StrategyModule;
import ExchangeMapModule;

namespace Agis
{


struct HydraPrivate
{
	ExchangeMap exchanges;
	ankerl::unordered_dense::map<std::string, Portfolio*> portfolios;
	ankerl::unordered_dense::map<std::string, Strategy*> strategies;
	Portfolio master_portfolio;
	tbb::task_group pool;
	bool built = false;

	HydraPrivate()
		: exchanges()
		, pool()
		, master_portfolio(pool,"master",0,std::nullopt,std::nullopt,0.0f)
	{
	}

};


//============================================================================
Hydra::Hydra()
{
	_p = new HydraPrivate();
	_p->master_portfolio._exchange_map = &_p->exchanges;
}


//============================================================================
Hydra::~Hydra()
{
	delete _p;
}


//============================================================================
std::expected<bool, AgisException> 
Hydra::run() noexcept
{
	auto lock = std::unique_lock(_mutex);
	if(!_p->built)
	{
		AGIS_ASSIGN_OR_RETURN(res, build());
	}
	this->reset();
	auto index = _p->exchanges.get_dt_index();
	for (size_t i = 0; i < index.size(); ++i)
	{
		AGIS_ASSIGN_OR_RETURN(res, step());
	}
	return true;
}


//============================================================================
std::expected<bool, AgisException>
Hydra::run_to(long long dt) noexcept
{
	auto global_time = _p->exchanges.get_global_time();
	while (global_time < dt)
	{
		AGIS_ASSIGN_OR_RETURN(res, step());
		global_time = _p->exchanges.get_global_time();

	}
	return true;
}


//============================================================================
std::expected<bool, AgisException>
Hydra::build() noexcept
{
	AGIS_ASSIGN_OR_RETURN(res, _p->exchanges.build());
	_p->built = true;
	return true;
}

std::expected<bool, AgisException>
Hydra::step() noexcept
{
	// step assets and exchanges forward in time
	_p->exchanges.step();

	// evaluate master portfolio at current time and prices
	_p->master_portfolio.evaluate(true, true);
	
	// step portfolios forward in time and call strategy next as needed
	AGIS_ASSIGN_OR_RETURN(res,_p->master_portfolio.step());
	_p->pool.wait();

	// process any open orders
	_p->exchanges.process_orders(true);

	// evaluate master portfolio at current time and prices
	_p->master_portfolio.evaluate(true, false);

	return true;
}


//============================================================================
std::expected<bool, AgisException>
Hydra::reset() noexcept
{
	_p->exchanges.reset();
	_p->master_portfolio.reset();
	return true;
}


//============================================================================
std::optional<Strategy*>
Hydra::get_strategy_mut(std::string const& strategy_id) const noexcept
{
	auto lock = std::shared_lock(_mutex);
	auto it = _p->strategies.find(strategy_id);
	if (it == _p->strategies.end()) return std::nullopt;
	return it->second;
}


//============================================================================
std::optional<Portfolio const*>
Hydra::get_portfolio(std::string const& portfolio_id) const noexcept
{
	auto lock = std::shared_lock(_mutex);
	if(portfolio_id == "master") return &_p->master_portfolio;
	auto it = _p->portfolios.find(portfolio_id);
	if (it == _p->portfolios.end()) return std::nullopt;
	return it->second;
}

//============================================================================
ExchangeMap &
Hydra::get_exchanges() const noexcept
{
	auto lock = std::shared_lock(_mutex);
	return _p->exchanges;
}


//============================================================================
std::optional<Exchange const*>
Hydra::get_exchange(std::string const& exchange_id) const noexcept
{
	auto lock = std::shared_lock(_mutex);
	auto res = _p->exchanges.get_exchange(exchange_id);
	if(res) return res.value();
	return std::nullopt;
}


//============================================================================
std::vector<long long>
const& Hydra::get_dt_index() const noexcept
{
	auto lock = std::shared_lock(_mutex);
	return _p->exchanges.get_dt_index();
}


//============================================================================
bool
Hydra::asset_exists(std::string const& asset_id) const noexcept
{
	auto lock = std::shared_lock(_mutex);
	return _p->exchanges.asset_exists(asset_id);
}


//============================================================================
std::expected<bool, AgisException>
Hydra::register_strategy(std::unique_ptr<Strategy> strategy)
{
	auto lock = std::unique_lock(_mutex);
	_p->strategies[strategy->get_strategy_id()] = strategy.get();
	auto portfolio = strategy->get_portfolio();
	return portfolio->add_strategy(std::move(strategy));
}

//============================================================================
std::expected<Portfolio*, AgisException>
Hydra::create_portfolio(
	std::string portfolio_id,
	std::optional<std::string> exchange_id,
	std::optional<Portfolio*> parent,
	double cash)
{
	auto lock = std::unique_lock(_mutex);
	if (_p->portfolios.count(portfolio_id))
	{
		return std::unexpected<AgisException>(AgisException("Portfolio already exists"));
	}

	// if no parent is specified, use the master portfolio
	if(!parent.has_value())
	{
		parent = &_p->master_portfolio;
	}
	// find the exchange
	std::optional<Exchange*> exchange = std::nullopt;
	if (exchange_id.has_value())
	{
		auto exchange_opt = _p->exchanges.get_exchange_mut(exchange_id.value());
		if (!exchange_opt) return std::unexpected<AgisException>(exchange_opt.error());
		exchange = exchange_opt.value();
	}

	auto res = parent.value()->add_child_portfolio(
		portfolio_id,
		_p->portfolios.size() + 1,
		exchange,
		cash
	);
	if (!res) return std::unexpected<AgisException>(res.error());
	_p->portfolios[portfolio_id] = res.value();
	return res.value();
}

//============================================================================
std::expected<Exchange const*, AgisException>
Hydra::create_exchange(std::string exchange_id, std::string dt_format, std::string source)
{
	auto lock = std::unique_lock(_mutex);
	_p->built = false;
	return _p->exchanges.create_exchange(exchange_id, dt_format, source);
}


}