module;
#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif

#include "AgisDeclare.h"

export module StrategyModule;

import <string>;
import <expected>;
import <optional>;

import AgisError;
import StrategyTracerModule;

namespace Agis
{


export class Strategy
{
	friend class Portfolio;
	friend class StrategyTracers;
	friend class Position;
	friend class Trade;
	friend class Hydra;

private:
	StrategyPrivate* _p;
	StrategyTracers _tracers;
	std::optional<AgisException> _exception;
	Exchange const& _exchange;
	
	void add_trade(Trade const* trade);
	void remove_trade(size_t asset_index);

	bool has_exception() const noexcept { return _exception.has_value(); }
	void set_exception(AgisException&& exception) noexcept { _exception = exception; }

	Strategy(
		std::string id,
		size_t index,
		double cash,
		Exchange const& exchange,
		Portfolio& portfolio
	);


protected:
	void place_market_order(size_t asset_index, double units);

	size_t get_strategy_index() const noexcept;
	double get_cash() const noexcept;
	Portfolio* get_portfolio() const noexcept;

	void reset() noexcept;
	virtual std::expected<bool,AgisException> step() = 0;


};

}