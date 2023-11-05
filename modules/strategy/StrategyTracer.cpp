module;

#include "AgisDeclare.h"
#include "AgisMacros.h"

module StrategyTracerModule;

import PortfolioModule;
import StrategyModule;

import <string>;


namespace Agis
{

//============================================================================
StrategyTracers::StrategyTracers(Strategy* strategy_, double cash)
{
	this->strategy = strategy_;
	this->starting_cash.store(cash);
	this->cash.store(cash);
	this->nlv.store(cash);
	this->set(Tracer::CASH);
	this->set(Tracer::NLV);
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
	this->cash_history.clear();
	this->nlv_history.clear();

	this->cash.store(this->starting_cash);
	this->nlv.store(this->cash.load());
	this->unrealized_pnl.store(0.0);
}


//============================================================================
void StrategyTracers::starting_cash_add_assign(double v) noexcept
{
	this->starting_cash.fetch_add(v);
	this->cash.fetch_add(v);
	if (portfolio)
	{
		auto parent = portfolio->get_parent_portfolio();
		if(!parent) return;
		auto p = parent.value();
		p->_tracers.starting_cash_add_assign(v);
		p->_tracers.cash_add_assign(v);
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
		portfolio->_tracers.unrealized_pnl_add_assign(v);
	}
}


//============================================================================
std::expected<bool, AgisException> StrategyTracers::evaluate()
{
	// Note: at this point all trades have been evaluated and the cash balance has been updated
	// so we only have to observer the values or use them to calculate other values.
	if (this->has(Tracer::NLV)) this->nlv_history.push_back(
		this->nlv.load()
	);
	if (this->has(Tracer::CASH)) this->cash_history.push_back(this->cash.load());

	if (portfolio)
	{
		for(auto& [k, v] : portfolio->_child_portfolios)
		{
			AGIS_ASSIGN_OR_RETURN(res, v->_tracers.evaluate());
		}
		for (auto& [k, v] : portfolio->_strategies)
		{
			AGIS_ASSIGN_OR_RETURN(res, v->_tracers.evaluate());
		}
	}

	return true;
}

//============================================================================
void StrategyTracers::zero_out_tracers()
{
	this->nlv.store(this->cash.load());
	this->unrealized_pnl.store(0.0);

}


}