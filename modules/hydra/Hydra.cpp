module;

#include "AgisMacros.h"
#include "AgisDeclare.h"
#include <tbb/task_group.h>

module HydraModule;

import PortfolioModule;
import StrategyModule;
import ExchangeMapModule;
import ExchangeModule;

namespace Agis
{


struct HydraPrivate
{
	ExchangeMap exchanges;
	std::unordered_map<std::string, Portfolio*> portfolios;
	std::unordered_map<std::string, Strategy*> strategies;
	Portfolio master_portfolio;
	size_t current_index = 0;
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
long long
Hydra::get_next_global_time() const noexcept
{
	return _p->exchanges.get_next_time();
}

//============================================================================
long long
Hydra::get_global_time() const noexcept
{
	return _p->exchanges.get_global_time();
}


//============================================================================
std::expected<bool, AgisException> 
Hydra::run() noexcept
{
	// build and reset all members as needed 
	if(!_p->built)
	{
		AGIS_ASSIGN_OR_RETURN(res, build());
	}
	if (_p->current_index == 0) this->reset();

	// lock the mutex
	_mutex.lock();
	auto index = _p->exchanges.get_dt_index();
	for (size_t i = _p->current_index; i < index.size(); ++i)
	{
		AGIS_ASSIGN_OR_RETURN(res, step());
	}
	_mutex.unlock();
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
	auto lock = std::unique_lock(_mutex);
	AGIS_ASSIGN_OR_RETURN(res, _p->exchanges.build());
	_p->built = true;
	_state = HydraState::BUILT;
	return true;
}

std::expected<bool, AgisException>
Hydra::step() noexcept
{
	if (_p->current_index == _p->exchanges.get_dt_index().size())
	{
		return std::unexpected<AgisException>(AgisException("End of data"));
	}

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

	_p->current_index++;
	if (_p->current_index == _p->exchanges.get_dt_index().size())
	{
		_state = HydraState::FINISHED;
	}
	return true;
}


//============================================================================
std::expected<bool, AgisException>
Hydra::reset() noexcept
{
	auto lock = std::unique_lock(_mutex);
	_p->exchanges.reset();
	_p->master_portfolio.reset();
	_p->current_index = 0;
	return true;
}


//============================================================================
std::optional<Strategy const*>
Hydra::get_strategy(std::string const& strategy_id) const noexcept
{
	return get_strategy_mut(strategy_id);
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
std::optional<Portfolio*>
Hydra::get_portfolio_mut(std::string const& portfolio_id) const noexcept
{
	auto lock = std::shared_lock(_mutex);
	if (portfolio_id == "master") return &_p->master_portfolio;
	auto it = _p->portfolios.find(portfolio_id);
	if (it == _p->portfolios.end()) return std::nullopt;
	return it->second;
}

//============================================================================
std::optional<Portfolio const*>
Hydra::get_portfolio(std::string const& portfolio_id) const noexcept
{
	return get_portfolio_mut(portfolio_id);
}

//============================================================================
ExchangeMap const&
Hydra::get_exchanges() const noexcept
{
	auto lock = std::shared_lock(_mutex);
	return _p->exchanges;
}


//============================================================================
std::unordered_map<std::string, Strategy*> const&
Hydra::get_strategies() const noexcept
{
	auto lock = std::shared_lock(_mutex);
	return _p->strategies;
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
std::optional<Exchange*>
Hydra::get_exchange_mut(std::string const& exchange_id) const noexcept
{
	auto lock = std::shared_lock(_mutex);
	auto res = _p->exchanges.get_exchange_mut(exchange_id);
	if (res) return res.value();
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
	if (_p->strategies.count(strategy->get_strategy_id()))
	{
		return std::unexpected<AgisException>(AgisException("Strategy already exists"));
	}
	_p->strategies[strategy->get_strategy_id()] = strategy.get();
	auto portfolio = strategy->get_portfolio_mut();
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
	if (exchange_id.has_value())
	{
		auto exchange_opt = _p->exchanges.get_exchange_mut(exchange_id.value());
		if (!exchange_opt) return std::unexpected<AgisException>(exchange_opt.error());
		auto exchange = std::move(exchange_opt.value());
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
	else
	{
		return std::unexpected<AgisException>(AgisException("Exchange not found"));
	}
}


//============================================================================
std::expected<Exchange const*, AgisException>
Hydra::create_exchange(
	std::string exchange_id,
	std::string dt_format,
	std::string source,
	std::optional<std::vector<std::string>> symbols)
{
	auto lock = std::unique_lock(_mutex);
	_p->built = false;
	return _p->exchanges.create_exchange(exchange_id, dt_format, source, symbols);
}


}