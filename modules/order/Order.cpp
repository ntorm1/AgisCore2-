module;

#include "AgisDeclare.h"
#include "AgisMacros.h"

module OrderModule;



namespace Agis
{

std::atomic<size_t> Order::order_counter(0);


//============================================================================
void Order::init(OrderType order_type, size_t asset_index, double units, Strategy* strategy, size_t strategy_index, size_t exchange_index, size_t portfolio_index)
{
	_type = order_type;
	_asset_index = asset_index;
	_units = units;
	_strategy = strategy;
	_strategy_index = strategy_index;
	_portfolio_index = portfolio_index;
	_exchange_index = exchange_index;
}


//============================================================================
void Order::reset()
{
	_asset = nullptr;
	_strategy = nullptr;
	parent_portfolio = std::nullopt; 
	_type = OrderType::UNKNOWN;
	_state = OrderState::PENDING;
	_force_close = false;

	_id = 0;
	_units = 0;
	_fill_price = 0;

	_create_time = 0;
	_fill_time = 0;
	_cancel_time = 0;

	_asset_index = 0;
	_strategy_index = 0;
	_portfolio_index = 0;
	_exchange_index = 0;
	_broker_index = 0;
}


//============================================================================
Order::Order(OrderType order_type,
	size_t asset_index,
	double units,
	Strategy* strategy,
	size_t strategy_index,
	size_t exchange_index,
	size_t portfolio_index
)
{
	_id = order_counter++;
	_type = order_type;
	_asset_index = asset_index;
	_units = units;
	_strategy = strategy;
	_strategy_index = strategy_index;
	_portfolio_index = portfolio_index;
	_exchange_index = exchange_index;
}


//============================================================================
void Order::fill(Asset const* asset, double avg_price_, long long fill_time)
{
	_asset = asset;
	_fill_price = avg_price_;
	_fill_time = fill_time;
	_state = OrderState::FILLED;
}


//============================================================================
void Order::cancel(long long order_cancel_time_)
{
	_cancel_time = order_cancel_time_;
	_state = OrderState::CANCELED;
}


//============================================================================
void Order::reject(long long reject_time)
{
	_cancel_time = reject_time;
	_state = OrderState::REJECTED;
}


}