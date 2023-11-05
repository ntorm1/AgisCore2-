module;
#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif

#include "AgisDeclare.h"
#include <ankerl/unordered_dense.h>

export module PositionModule;

import <atomic>;
import <optional>;

namespace Agis
{

export class Position
{
    friend class Portfolio;
private:
	static std::atomic<size_t> _position_counter;

	Asset const& _asset;
    double _units;
    double _avg_price;
    double _open_price;
    double _close_price;
    double _last_price;
    double _nlv;
    double _unrealized_pnl;
    double _realized_pnl;

    long long _open_time;
    long long _close_time;
    size_t _bars_held;

    size_t _position_id;
    size_t _asset_index;
    size_t _strategy_index;
    size_t _portfolio_index;

    /// <summary>
    /// Mapping between strategy id and trade object.
    /// </summary>
    ankerl::unordered_dense::map<size_t, std::unique_ptr<Trade>> _trades;

    void adjust(
        Strategy* strategy,
        Order* order,
        std::vector<std::unique_ptr<Trade>>& trades
    ) noexcept;
    void close(Order const* order) noexcept;
    void evaluate(bool on_close, bool is_reprice) noexcept;
    bool is_last_row();
    std::unique_ptr<Order> generate_position_inverse_order() const noexcept;
    auto& get_trades() noexcept { return this->_trades; }
    std::optional<Trade*> get_trade(size_t strategy_index) const noexcept;
    double get_unrealized_pnl() const noexcept { return this->_unrealized_pnl; }

public:
    Position(
        Strategy* strategy,
        Order const* order
    ) noexcept;

    [[nodiscard]] double get_nlv() const { return this->_nlv; }
    [[nodiscard]] double get_unrealized_pl() const { return this->_unrealized_pnl; }
    [[nodiscard]] double get_realized_pl() const { return this->_realized_pnl; }
    [[nodiscard]] double get_units() const { return this->_units; }
    [[nodiscard]] bool trade_exists(size_t strategy_index) const noexcept;

};

}