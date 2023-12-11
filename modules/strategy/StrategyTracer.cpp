module;

#include "AgisDeclare.h"
#include "AgisMacros.h"
#include <Eigen/Dense>
module StrategyTracerModule;

import PortfolioModule;
import StrategyModule;

import <string>;


namespace Agis
{

//============================================================================
StrategyTracers::StrategyTracers(Strategy* strategy_, double cash, size_t asset_count, size_t exchange_offset)
{
	this->strategy = strategy_;
	this->starting_cash.store(cash);
	this->cash.store(cash);
	this->nlv.store(cash);
	this->set(Tracer::CASH);
	this->set(Tracer::NLV);
	_exchange_offset = exchange_offset;
	_weights.resize(asset_count);
	_weights.setZero();
};


//============================================================================
StrategyTracers::StrategyTracers(Portfolio* portfolio, double cash)
{
	this->portfolio = portfolio;
	this->starting_cash.store(cash);
	this->cash.store(cash);
	this->nlv.store(cash);
	this->set(Tracer::CASH);
	this->set(Tracer::NLV);
};


//============================================================================
std::optional<double> StrategyTracers::get(Tracer tracer) const
{
	switch (tracer)
	{
	case Tracer::CASH:
		return this->cash.load();
		break;
	case Tracer::NLV:
		return this->nlv.load();
		break;
	}
	return std::nullopt;
}


//============================================================================
void StrategyTracers::reset()
{
	std::fill(this->cash_history.begin(), this->cash_history.end(), 0.0);
	std::fill(this->nlv_history.begin(), this->nlv_history.end(), 0.0);

	this->cash.store(this->starting_cash);
	this->nlv.store(this->cash.load());
	this->unrealized_pnl.store(0.0);
	this->_current_index = 0;
}


//============================================================================
void
StrategyTracers::build(size_t n)
{
	this->cash_history = std::vector<double>(n, 0.0);
	this->nlv_history = std::vector<double>(n, 0.0);
}


//============================================================================
void StrategyTracers::starting_cash_add_assign(double v) noexcept
{
	this->starting_cash.fetch_add(v);
	this->cash.fetch_add(v);
	this->nlv.fetch_add(v);
	if (portfolio)
	{
		auto parent = portfolio->get_parent_portfolio();
		if(!parent) return;
		auto p = parent.value();
		p->_tracers.starting_cash_add_assign(v);
	}
}

void StrategyTracers::cash_add_assign(double v) noexcept
{
	this->cash.fetch_add(v); 
	if (portfolio)
	{
		auto parent = portfolio->get_parent_portfolio();
		if (!parent) return;
		auto p = parent.value();
		p->_tracers.cash_add_assign(v);
	}
}


//============================================================================
void StrategyTracers::nlv_add_assign(double v) noexcept
{
	this->nlv.fetch_add(v); 
	if (portfolio)
	{
		auto parent = portfolio->get_parent_portfolio();
		if (!parent) return;
		auto p = parent.value();
		p->_tracers.nlv_add_assign(v);
	}
}


//============================================================================
void StrategyTracers::unrealized_pnl_add_assign(double v) noexcept
{
	this->unrealized_pnl.fetch_add(v);
	if (portfolio)
	{
		auto parent = portfolio->get_parent_portfolio();
		if (!parent) return;
		parent.value()->_tracers.unrealized_pnl_add_assign(v);
	}
}


//============================================================================
std::optional<Tracer>
StrategyTracers::get_type(std::string const& name) const noexcept
{
	if (_tracer_map().contains(name))
	{
		return _tracer_map().at(name);
	}
	return std::nullopt;
}


//============================================================================
std::optional<std::vector<double>const*>
StrategyTracers::get_column(Tracer t) const noexcept
{
	if (!this->has(t)) return std::nullopt;
	switch (t)
	{
	case Tracer::CASH:
		return &this->cash_history;
		break;
	case Tracer::NLV:
		return &this->nlv_history;
		break;
	}
	return std::nullopt;
}


//============================================================================
std::unordered_map<std::string, Tracer> const& StrategyTracers::_tracer_map() noexcept
{
	static std::unordered_map<std::string, Tracer> _map = {
		{"Cash", Tracer::CASH},
		{"Nlv", Tracer::NLV}
	};
	return _map;
}


//============================================================================
std::expected<bool, AgisException> StrategyTracers::evaluate(bool is_reprice)
{
	// at this point weights vector contains the nlv of each trade. To get the strategy 
	// portfolio weights we need to divide by the nlv of the evaluated strategy.
	if(strategy && is_reprice)
	{
		_weights /= this->nlv.load();
	}

	if (!is_reprice) 
	{
		//TODO strategy added after build
		// Note: at this point all trades have been evaluated and the cash balance has been updated
		// so we only have to observer the values or use them to calculate other values.
		if (this->has(Tracer::NLV)) this->nlv_history[_current_index] = this->nlv.load();
		if (this->has(Tracer::CASH)) this->cash_history[_current_index] = this->cash.load();
		_current_index++;
	}

	if (portfolio)
	{
		for(auto& [k, v] : portfolio->_child_portfolios)
		{
			AGIS_ASSIGN_OR_RETURN(res, v->_tracers.evaluate(is_reprice));
		}
		for (auto& [k, v] : portfolio->_strategies)
		{
			AGIS_ASSIGN_OR_RETURN(res, v->_tracers.evaluate(is_reprice));
		}
	}
	return true;
}

//============================================================================
void StrategyTracers::zero_out_tracers()
{
	nlv.store(cash.load());
	_weights.setZero();
	unrealized_pnl.store(0.0);

}


}