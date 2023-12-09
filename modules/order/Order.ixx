module;

#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif

#include "AgisDeclare.h"

export module OrderModule;

import <string>;
import <atomic>;
import <optional>;


namespace Agis
{

export enum class OrderType : uint8_t
{
	UNKNOWN,
	MARKET_ORDER
};

export enum class OrderState
{
	PENDING,  /// order has been created but yet to be sent
	OPEN,     /// order is open on the exchange
	FILLED,   /// order has been filled by the exchange
	CANCELED, /// order has been canceled by a strategy
	REJECTED, /// order was rejected 
	CHEAT,    /// allows orders we know will fill to be filled and processed in single router call
};

class OrderFactory;

export class Order
{
	friend class ExchangeMap;
	friend class Exchange;
	friend class Portfolio;
	friend class Position;
private:
	static std::atomic<size_t> order_counter;
	Asset const* _asset = nullptr;
	Strategy*	_strategy = nullptr;
	std::optional<Portfolio*> parent_portfolio;
	OrderType	_type;
	OrderState	_state = OrderState::PENDING;
	bool		_force_close = false;

	size_t		_id = 0;
	double		_units = 0;
	double		_fill_price = 0;

	long long _create_time = 0;
	long long _fill_time = 0;
	long long _cancel_time = 0;

	size_t _asset_index = 0;
	size_t _strategy_index = 0;
	size_t _portfolio_index = 0;
	size_t _exchange_index = 0;
	size_t _broker_index = 0;

	void fill(Asset const* asset, double market_price, long long fill_time);
	void cancel(long long cancel_time);
	void reject(long long reject_time);
	void set_order_state(OrderState state) noexcept { this->_state = state; }
	void set_units(double units) noexcept { this->_units = units; }
	void set_force_close(bool force_close) noexcept { this->_force_close = force_close; }
	[[nodiscard]] inline Strategy* get_strategy_mut() const noexcept { return this->_strategy; }
	[[nodiscard]] inline std::optional<Portfolio*> get_parent_portfolio_mut() const noexcept { return this->parent_portfolio; }

public:
	Order(){}
	void init (OrderType order_type,
		size_t asset_index,
		double units,
		Strategy* strategy,
		size_t strategy_index,
		size_t exchange_index,
		size_t portfolio_index
	);
	void reset();
	Order(OrderType order_type,
		size_t asset_index,
		double units,
		Strategy* strategy,
		size_t strategy_index,
		size_t exchange_index,
		size_t portfolio_index
	);
	void set_parent_portfolio(Portfolio* portfolio) noexcept { this->parent_portfolio = portfolio; }
	[[nodiscard]] inline bool is_force_close() const noexcept { return this->_force_close; }
	[[nodiscard]] inline Strategy const* get_strategy() const noexcept { return this->_strategy; }
	[[nodiscard]] inline Asset const* get_asset() const noexcept { return this->_asset; }
	[[nodiscard]] inline size_t get_order_id() const noexcept { return this->_id; }
	[[nodiscard]] inline size_t get_exchange_index() const noexcept { return this->_exchange_index; }
	[[nodiscard]] inline size_t get_asset_index() const noexcept { return this->_asset_index; }
	[[nodiscard]] inline size_t get_strategy_index() const noexcept { return this->_strategy_index; }
	[[nodiscard]] inline size_t get_portfolio_index() const noexcept { return this->_portfolio_index; }
	[[nodiscard]] inline size_t get_broker_index() const noexcept { return this->_broker_index; }
	[[nodiscard]] inline OrderType get_order_type() const noexcept { return this->_type; }
	[[nodiscard]] inline OrderState get_order_state() const noexcept { return this->_state; }
	[[nodiscard]] inline double get_units() const noexcept { return this->_units; }
	[[nodiscard]] inline double get_fill_price() const noexcept { return this->_fill_price; }
	[[nodiscard]] inline long long get_create_time() const noexcept { return this->_create_time; }
	[[nodiscard]] inline long long get_fill_time() const noexcept { return this->_fill_time; }
	[[nodiscard]] inline long long get_cancel_time() const noexcept { return this->_cancel_time; }

};

}