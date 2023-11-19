module;
#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif
#include <tbb/task_group.h>
#include "AgisDeclare.h"
#include <ankerl/unordered_dense.h>

export module PortfolioModule;


import <optional>;
import <expected>;
import <shared_mutex>;

import AgisError;
import ExchangeModule;
import StrategyTracerModule;

namespace Agis
{

export class Portfolio
{
	friend class Hydra;
	friend class Exchange;
	friend class Trade;
	friend class StrategyTracers;
	friend class StrategyPrivate;
private:
	mutable std::shared_mutex _mutex;
	size_t		_portfolio_index;
	bool _step_call = false;
	std::string _portfolio_id;

	ankerl::unordered_dense::map<size_t, std::unique_ptr<Position>>		_positions;
	ankerl::unordered_dense::map<size_t, std::unique_ptr<Portfolio>>	_child_portfolios;
	ankerl::unordered_dense::map<size_t, std::unique_ptr<Strategy>>		_strategies;

	tbb::task_group&			_task_group;
	std::optional<Portfolio*>	_parent_portfolio;
	std::optional<ExchangeMap*>	_exchange_map = std::nullopt;
	std::optional<Exchange*>		_exchange = std::nullopt;

	std::vector<Trade*>						_trade_history;
	std::vector<std::unique_ptr<Position>>	_position_history;
	std::vector<std::unique_ptr<Order>>		_order_history;

	StrategyTracers _tracers;

	void reset();
	void zero_out();
	std::expected<bool, AgisException> evaluate(bool on_close, bool is_reprice);
	[[nodiscard]] std::expected<bool, AgisException> step();


	std::expected<Portfolio*, AgisException> add_child_portfolio(
		std::string id,
		size_t index,
		std::optional<Exchange*>	exchange,
		double cash
	);
	std::expected<bool, AgisException> add_strategy(std::unique_ptr<Strategy>);
	void place_order(std::unique_ptr<Order> order) noexcept;
	void process_filled_order(Order* order);
	void process_order(std::unique_ptr<Order> order);

	void close_position(Order const* order, Position* position) noexcept;
	void close_trade(size_t asset_index, size_t strategy_index) noexcept;
	void insert_trade(Trade* trade) noexcept;
	void adjust_position(Order* order, Position* position) noexcept;
	void open_position(Order const* order) noexcept;
	void open_position(Trade* trade) noexcept;

	size_t get_asset_count_limit() const noexcept;
	std::optional<Portfolio*> get_parent_portfolio() const noexcept { return _parent_portfolio;}
	std::optional<Exchange const*> get_exchange() const noexcept { return _exchange; }
	void set_child_portfolio_position_parents(Position* p) noexcept;
	std::optional<Position*> get_parent_position(size_t asset_index) const noexcept;
	std::optional<Position*> get_position_mut(size_t asset_index) const noexcept;
	std::optional<Trade*> get_trade_mut(size_t asset_index, size_t strategy_index) const noexcept;

public:
	Portfolio(
		tbb::task_group& _thread_pool,
		std::string id,
		size_t index,
		std::optional<Exchange*>	exchange,
		std::optional<Portfolio*>	parent_portfolio,
		double cash
	);
	~Portfolio();
	
	AGIS_API size_t get_portfolio_index() const noexcept { return _portfolio_index; }
	AGIS_API std::optional<Position const*> get_position(std::string const& asset_id) const noexcept;


	bool position_exists(size_t asset_index) const noexcept;
	bool trade_exists(size_t asset_index, size_t strategy_index) const noexcept;
	AGIS_API double get_cash() const noexcept;
	AGIS_API double get_nlv() const noexcept;

};


}