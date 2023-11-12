module;

#include "AgisDeclare.h"
#include "AgisMacros.h"

module TradeModule;

import StrategyModule;
import OrderModule;
import PortfolioModule;
import PositionModule;

namespace Agis
{

std::atomic<size_t> Trade::_trade_counter(0);


Trade::Trade(Strategy* strategy, Order const* order, Position* parent_position) noexcept
	: _asset(*order->get_asset()), _parent_position(parent_position)
{
	_trade_id = _trade_counter++;
	_strategy = strategy;
	_portfolio = strategy->get_portfolio_mut();
	_units = order->get_units();
	_avg_price = order->get_fill_price();
	_open_price = _avg_price;
	_close_price = 0;
	_last_price = _avg_price;
	_nlv = _units * _avg_price;
	_unrealized_pnl = 0;
	_realized_pnl = 0;

	_open_time = order->get_fill_time();
	_close_time = 0;
	_bars_held = 0;
	
	_asset_index = order->get_asset_index();
	_strategy_index = order->get_strategy_index();
	_portfolio_index = order->get_portfolio_index();
	
}


//============================================================================
void Trade::close(Order const* filled_order)
{
	_close_price = filled_order->get_fill_price();
	_close_time = filled_order->get_fill_time();
	_realized_pnl += (_units  * (_close_price - _avg_price));
	_realized_pnl = 0;
	// clear the trade from the parent strategy's trade map
	_strategy->remove_trade(_asset_index);
}


//============================================================================
void Trade::increase(Order const* filled_order)
{
	auto units_ = filled_order->get_units();
	auto p = filled_order->get_fill_price();
	double new_units = (abs(_units) + abs(units_));
	_avg_price = ((abs(_units) * _avg_price) + (abs(units_) * p)) / new_units;
	_units += units_;
}


//============================================================================
void Trade::reduce(Order const* filled_order)
{
	auto units_ = filled_order->get_units();
	auto adjustment = -1 * (units_ * (filled_order->get_fill_price() - _avg_price));
	_realized_pnl += adjustment;
	_units += units_;
}


//============================================================================
void Trade::adjust(Order const* filled_order)
{
	// extract order information
	auto order_units = filled_order->get_units();
	if (order_units * _units > 0)
	{
		this->increase(filled_order);
	}
	else 
	{
		this->reduce(filled_order);
	}
	_nlv = _units * filled_order->get_fill_price();
}


//============================================================================
void
Trade::evaluate(double market_price, bool on_close, bool is_reprice)
{
	auto nlv_new = _units * market_price;
	auto unrealized_pl_new = _units * (market_price - _avg_price);
	auto nlv_adjustment = nlv_new - _nlv;
	auto unrealized_pl_adjustment = unrealized_pl_new - _unrealized_pnl;
	_parent_position->_nlv += nlv_adjustment;
	_parent_position->_unrealized_pnl += unrealized_pl_adjustment;

	// adjust strategy levels 
	_strategy->_tracers.nlv_add_assign(nlv_new);
	_strategy->_tracers.unrealized_pnl_add_assign(unrealized_pl_new - _unrealized_pnl);
	_portfolio->_tracers.nlv_add_assign(nlv_new);
	_portfolio->_tracers.unrealized_pnl_add_assign(unrealized_pl_new - _unrealized_pnl);
	
	_nlv = nlv_new;
	_unrealized_pnl = unrealized_pl_new;
	_last_price = market_price;
	if (on_close && !is_reprice) { _bars_held++; }
}

UniquePtr<Order> Trade::generate_trade_inverse()
{
	auto order = std::make_unique<Order>(
		OrderType::MARKET_ORDER,
		_asset_index,
		-1 * _units,
		_strategy,
		0,
		0,
		0
	);
	order->set_parent_portfolio(_portfolio);
	return std::move(order);
}


}