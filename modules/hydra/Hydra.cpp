module;

#include "AgisMacros.h"
#include "AgisDeclare.h"
#include <tbb/task_group.h>
#include <ankerl/unordered_dense.h>

module HydraModule;

import PortfolioModule;
import ExchangeMapModule;

namespace Agis
{


struct HydraPrivate
{
	ExchangeMap exchanges;
	ankerl::unordered_dense::map<std::string, Portfolio*> portfolios;
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
Hydra::run(std::string const& path) noexcept
{
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
std::optional<Portfolio const*>
Hydra::get_portfolio(std::string const& portfolio_id) const noexcept
{
	auto it = _p->portfolios.find(portfolio_id);
	if (it == _p->portfolios.end()) return std::nullopt;
	return it->second;
}

//============================================================================
ExchangeMap const&
Hydra::get_exchanges() const noexcept
{
	return _p->exchanges;
}


//============================================================================
std::vector<long long>
const& Hydra::get_dt_index() const noexcept
{
	return _p->exchanges.get_dt_index();
}


//============================================================================
bool
Hydra::asset_exists(std::string const& asset_id) const noexcept
{
	return _p->exchanges.asset_exists(asset_id);
}


//============================================================================
std::expected<Portfolio const*, AgisException>
Hydra::create_portfolio(
	std::string portfolio_id,
	std::optional<std::string> exchange_id,
	std::optional<Portfolio*> parent,
	double cash)
{
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
	_p->built = false;
	return _p->exchanges.create_exchange(exchange_id, dt_format, source);
}


}