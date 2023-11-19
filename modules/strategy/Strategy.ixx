module;
#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif
#include <Eigen/Dense>
#include "AgisDeclare.h"

export module StrategyModule;

import <atomic>;
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
	static std::atomic<size_t> _strategy_counter;
	std::string _strategy_id;
	StrategyPrivate* _p;
	StrategyTracers _tracers;
	std::optional<AgisException> _exception;
	Exchange const& _exchange;
	
	void add_trade(Trade const* trade);
	void remove_trade(size_t asset_index);

	bool has_exception() const noexcept { return _exception.has_value(); }
	void set_exception(AgisException&& exception) noexcept { _exception = exception; }
	size_t get_asset_count_limit() const noexcept;
	Portfolio* get_portfolio_mut() const noexcept;

protected:
	AGIS_API Strategy(
		std::string strategy_id,
		double cash,
		Exchange const& exchange,
		Portfolio& portfolio
	);

	[[nodiscard]] std::expected<bool,AgisException> set_allocation(
		Eigen::VectorXd& exchange_view,
		double epsilon,
		bool clear_missing = true
	) noexcept;

	std::optional<Trade*> get_trade_mut(size_t asset_index) const noexcept;
	AGIS_API void place_market_order(size_t asset_index, double units);
	AGIS_API size_t get_strategy_index() const noexcept;
	AGIS_API std::optional<size_t> get_asset_index(std::string const& asset_id);
	

	void reset() noexcept;
	virtual std::expected<bool,AgisException> step() = 0;

public:
	AGIS_API std::optional<Trade const*> get_trade(size_t asset_index) const noexcept;
	AGIS_API double get_cash() const noexcept;
	AGIS_API double get_nlv() const noexcept;
	AGIS_API size_t get_strategy_index();
	AGIS_API std::string const& get_strategy_id() const noexcept{ return _strategy_id; }

};

}