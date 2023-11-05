module;

#include "AgisDeclare.h"
#include "AgisMacros.h"
#include <cmath>

module PositionModule;

import <memory>;

import AssetModule;
import OrderModule;
import TradeModule;
import StrategyModule;

namespace Agis
{


std::atomic<size_t> Position::_position_counter(0);


//============================================================================
Position::Position(
	Strategy* strategy,
	Order const* order,
	std::optional<Position*> parent_position) noexcept
	: _asset(*order->get_asset()), parent_position(parent_position)
{
	_position_id = _position_counter++;
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

	auto trade = new Trade(
		strategy,
		order,
		this
	);
	_trades.insert({ _strategy_index, trade});
	strategy->add_trade(trade);
}


//============================================================================
Position::Position(
	Strategy* strategy,
	Trade* trade,
	std::optional<Position*> parent_position) noexcept
	: _asset(trade->get_asset()), parent_position(parent_position)
{
	_position_id = _position_counter++;
	_units = trade->get_units();
	_avg_price = trade->get_avg_price();
	_open_price = _avg_price;
	_close_price = 0;
	_last_price = _avg_price;
	_nlv = _units * _avg_price;
	_unrealized_pnl = 0;
	_realized_pnl = 0;

	_open_time = trade->get_open_time();
	_close_time = 0;
	_bars_held = 0;

	_asset_index = trade->get_asset_index();
	_strategy_index = trade->get_strategy_index();
	_portfolio_index = trade->get_portfolio_index();

	_trades.insert({ _strategy_index, trade });
	strategy->add_trade(trade);
}


//============================================================================
void
Position::adjust(Trade* trade) noexcept
{
	auto trade_units = trade->get_units();
	auto fill_price = trade->get_avg_price();
	// increasing position
	if (trade_units * _units > 0)
	{
		double new_units = abs(_units) + abs(trade_units);
		this->_avg_price = (
			(abs(this->_units) * this->_avg_price) +
			(abs(trade_units) * fill_price)
			);
		this->_avg_price /= new_units;

	}
	// reducing position
	else
	{
		this->_realized_pnl += (abs(trade_units) * (fill_price - this->_avg_price));
	}
	// adjust position units
	this->_units += trade_units;
	_trades.emplace(trade->get_strategy_index(), trade);
}


//============================================================================
void
Position::adjust(
	Strategy* strategy,
	Order* order,
	std::vector<Trade*>& trade_history) noexcept
{
	auto order_units = order->get_units();
	auto fill_price = order->get_fill_price();
	// increasing position
	if (order_units * _units > 0)
	{
		double new_units = abs(_units) + abs(order_units);
		this->_avg_price = (
			(abs(this->_units) * this->_avg_price) +
			(abs(order_units) * fill_price)
			);
		this->_avg_price /= new_units;
	}
	// reducing position
	else
	{
		this->_realized_pnl += (abs(order_units) * (fill_price - this->_avg_price));
	}
	// adjust position units
	this->_units += order_units;
	parent_adjust(order_units, fill_price);

	//if strategy does not have a trade, create one
	auto strategy_index = order->get_strategy_index();
	auto trade_opt = this->get_trade_mut(strategy_index);
	if (!trade_opt.has_value())
	{
		auto trade = new Trade(
			strategy,
			order,
			this
		);
		this->_trades.insert({ strategy_index,trade});
		strategy->add_trade(trade);
	}
	else
	{
		auto& trade = trade_opt.value();
		// position is closed
		if (abs(trade->get_units() + order_units) < UNIT_EPSILON)
		{
			trade->close(order);
			auto trade_ptr = _trades.at(strategy_index);
			_trades.erase(strategy_index);
			strategy->remove_trade(order->get_asset_index());
			trade_history.push_back(trade_ptr);
		}
		else
		{
			// test to see if the trade is being reversed
			auto trade_units = trade->get_units();
			if (std::signbit(_units) != std::signbit(trade_units)
				&& abs(_units) > abs(trade_units))
			{
				// get the number of units left over after reversing the trade
				auto units_left = trade_units + _units;

				trade->close(order);
				auto trade_ptr = _trades.at(strategy_index);
				_trades.erase(strategy_index);
				strategy->remove_trade(order->get_asset_index());
				trade_history.push_back(trade_ptr);

				// open a new trade with the new order minus the units needed to close out 
				// the previous trade
				order->set_units(units_left);
				auto trade = new Trade(
					strategy,
					order,
					this
				);
				this->_trades.insert({ strategy_index, trade });
				strategy->add_trade(trade);
				order->set_units(order_units);
			}
			else
			{
				trade->adjust(order);
			}
		}
	}
}


//============================================================================
void Position::parent_adjust(double units, double price) noexcept
{
	if (parent_position.has_value())
	{
		auto parent = parent_position.value();
		auto existing_units = parent->_units;
		auto existing_price = parent->_avg_price;
		auto new_units = existing_units + units;
		parent->_avg_price = (
			(abs(existing_units) * existing_price) +
			(abs(units) * price)
			);
		parent->_avg_price /= new_units;
		parent->_units += units;
		parent->_nlv = parent->_units * price;
		parent->_unrealized_pnl = parent->_units * (price - parent->_avg_price);
		if (units * existing_units < 0)
		{
			parent->_realized_pnl += (abs(units) * (price - parent->_avg_price));
		}
		parent->parent_adjust(units, price);
	}
}


//============================================================================
void
Position::close(
	Order const* order
) noexcept
{
	auto price = order->get_fill_price();
	_close_price = price;
	_close_time = order->get_fill_time();
	_last_price = price;
	_unrealized_pnl = 0;
	_realized_pnl = _units * (price - _avg_price);
	_nlv = _units * price;
	for (auto& trade : _trades)
	{
		trade.second->close(order);
	}
}


//============================================================================
void Position::evaluate(bool on_close, bool is_reprice) noexcept
{
	auto price_opt = _asset.get_market_price(on_close);
	if (!price_opt) return;
	_last_price = price_opt.value();
	_nlv = 0.0f;
	_unrealized_pnl = _units*(_last_price - _avg_price);
	if (on_close && !is_reprice) { this->_bars_held++; }

	for(auto&[strategy_index, trade] : _trades)
	{
		double trade_nlv = trade->get_nlv();
		trade->evaluate(_last_price, on_close, is_reprice);
		_nlv += trade->_nlv;

		// propgate nlv adjustment to parent positions
		auto parent = parent_position;
		while (parent)
		{
			parent_position.value()->_nlv += (trade->_nlv - trade_nlv);
			parent = parent_position.value()->parent_position;
		}
	}
}


bool Position::is_last_row()
{
	return (_asset.get_state() == AssetState::LAST);
}


//============================================================================
std::unique_ptr<Order>
Position::generate_position_inverse_order() const noexcept
{
	return std::make_unique<Order>(
		OrderType::MARKET_ORDER,
		_asset_index,
		-1 * _units,
		nullptr,
		0,
		0,
		0
	);
}


//============================================================================
void Position::close_trade(size_t strategy_index) noexcept
{
	auto it = _trades.find(strategy_index);
	if (it != _trades.end())
	{
		auto trade = it->second;
		_trades.erase(strategy_index);
		_units -= trade->get_units();
		_unrealized_pnl -= trade->get_unrealized_pnl();
		_realized_pnl += trade->get_realized_pnl();
		_nlv -= trade->get_nlv();
	}
}

//============================================================================
std::optional<Trade*> Position::get_trade_mut(size_t strategy_index) const noexcept
{
	auto it = _trades.find(strategy_index);
	if (it != _trades.end())
	{
		return it->second;
	}
	return std::nullopt;
}


//============================================================================
bool
Position::trade_exists(size_t strategy_index) const noexcept
{
	return _trades.find(strategy_index) != _trades.end();
}


//============================================================================
std::optional<Trade const*>
Position::get_trade(size_t strategy_index) const noexcept
{
	auto it = _trades.find(strategy_index);
	if (it != _trades.end())
	{
		return it->second;
	}
	return std::nullopt;
}

}
