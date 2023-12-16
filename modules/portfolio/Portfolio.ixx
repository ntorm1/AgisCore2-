module;
#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif
#include <tbb/task_group.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_vector.h>
#include <ankerl/unordered_dense.h>

#include "AgisDeclare.h"

export module PortfolioModule;

import <optional>;
import <expected>;
import <shared_mutex>;

import AgisError;
import ExchangeModule;
import StrategyTracerModule;

namespace Agis
{

class PortfolioPrivate;

export class Portfolio
{
	friend class Hydra;
	friend class Exchange;
	friend class Trade;
	friend class StrategyTracers;
	friend class StrategyPrivate;
private:
	PortfolioPrivate* _p = nullptr;
	mutable std::shared_mutex _mutex;
	size_t		_portfolio_index;
	bool _step_call = false;
	std::string _portfolio_id;

	tbb::concurrent_hash_map<size_t, Position*>		_positions;
	ankerl::unordered_dense::map<size_t, UniquePtr<Portfolio>>	_child_portfolios;
	ankerl::unordered_dense::map<size_t, UniquePtr<Strategy>>		_strategies;

	tbb::task_group&			_task_group;
	std::optional<Portfolio*>	_parent_portfolio;
	std::optional<ExchangeMap*>	_exchange_map = std::nullopt;
	std::optional<Exchange*>	_exchange = std::nullopt;
	StrategyTracers _tracers;

	void build(size_t n);
	void reset();
	void zero_out();
	void build_mutex_map() noexcept;
	std::mutex& get_asset_mutex(size_t asset_index) const noexcept;
	[[nodiscard]] std::expected<bool, AgisException> remove_strategy(Strategy& strategy);
	[[nodiscard]] std::expected<bool, AgisException> evaluate(bool on_close, bool is_reprice);
	[[nodiscard]] std::expected<bool, AgisException> step();

	template <typename T>
	void free_object_vector(tbb::concurrent_vector<T*>& objects);

	std::expected<Portfolio*, AgisException> add_child_portfolio(
		std::string id,
		size_t index,
		std::optional<Exchange*>	exchange,
		double cash
	);
	std::expected<bool, AgisException> add_strategy(UniquePtr<Strategy>);
	void place_order(UniquePtr<Order> order) noexcept;
	void process_filled_order(Order* order);
	void process_order(UniquePtr<Order> order);
	void remember_order(Order* order) noexcept;

	void close_position(Order const* order, Position* position) noexcept;
	void close_trade(size_t asset_index, size_t strategy_index) noexcept;
	void insert_trade(Trade* trade) noexcept;
	void adjust_position(Order* order, Position* position) noexcept;
	void open_position(Order const* order) noexcept;
	void open_position(Trade* trade) noexcept;

	size_t get_asset_count_limit() const noexcept;
	std::optional<Portfolio*> get_parent_portfolio() const noexcept { return _parent_portfolio;}
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
	AGIS_API std::unique_lock<std::shared_mutex> __aquire_write_lock() const noexcept;
	AGIS_API std::shared_lock<std::shared_mutex> __aquire_read_lock() const noexcept;
	AGIS_API size_t get_portfolio_index() const noexcept { return _portfolio_index; }
	AGIS_API std::optional<Position const*> get_position(std::string const& asset_id) const noexcept;
	AGIS_API std::string const& get_portfolio_id() const noexcept { return _portfolio_id; }
	AGIS_API auto const& child_portfolios() const noexcept { return _child_portfolios; }
	AGIS_API auto const& child_strategies() const noexcept { return _strategies; }
	AGIS_API double get_cash() const noexcept;
	AGIS_API double get_nlv() const noexcept;
	AGIS_API auto const& positions() const noexcept {return _positions;}
	AGIS_API tbb::concurrent_vector<Order*> const& order_history() const noexcept;
	AGIS_API auto const& get_tracers() const noexcept { return _tracers; }

	std::optional<Exchange const*> get_exchange() const noexcept { return _exchange; }
	bool position_exists(size_t asset_index) const noexcept;
	bool trade_exists(size_t asset_index, size_t strategy_index) const noexcept;


};

}