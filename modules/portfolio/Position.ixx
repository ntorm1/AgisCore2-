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
    friend class Trade;
    friend class Portfolio;
private:
	static std::atomic<size_t> _position_counter;
    std::optional<Position*> parent_position;
    Asset const* _asset = nullptr;
    double _units;
    double _avg_price = 0.0f;
    double _open_price = 0.0f;
    double _close_price = 0.0f;
    double _last_price = 0.0f;
    double _nlv = 0.0f;
    double _unrealized_pnl = 0.0f;
    double _realized_pnl = 0.0f;

    long long _open_time = 0;
    long long _close_time = 0;
    size_t _bars_held = 0;

    size_t _position_id = 0;
    size_t _asset_index = 0;
    size_t _strategy_index = 0;
    size_t _portfolio_index = 0;

    /// <summary>
    /// Mapping between strategy id and trade object.
    /// </summary>
    ankerl::unordered_dense::map<size_t, Trade*> _trades;

    void adjust(
        Strategy* strategy,
        Order* order,
        std::vector<Trade*>& trade_history
    ) noexcept;
    void parent_adjust(double units, double price) noexcept;
    void set_parent_position(Position* parent_position) noexcept { this->parent_position = parent_position;}
    void adjust(Trade* trade) noexcept;
    void close(Order const* order) noexcept;

    void evaluate(bool on_close, bool is_reprice) noexcept;
    bool is_last_row();
    std::unique_ptr<Order> generate_position_inverse_order() const noexcept;
    auto& get_trades() noexcept { return this->_trades; }
    void clear_trades() noexcept { this->_trades.clear(); }
    void close_trade(size_t strategy_index) noexcept;
    std::optional<Trade*> get_trade_mut(size_t strategy_index) const noexcept;

public:
    // memory pool functions
    Position();
    void init(
        Strategy* strategy,
        Order const* order,
        std::optional<Position*> parent_position = std::nullopt
    ) noexcept;
    void init(
        Strategy* strategy,
        Trade* trade,
        std::optional<Position*> parent_position = std::nullopt
    ) noexcept;
    void reset();


    Position(
        Strategy* strategy,
        Order const* order,
        std::optional<Position*> parent_position = std::nullopt
    ) noexcept;
    Position(
        Strategy* strategy,
        Trade* trade,
        std::optional<Position*> parent_position = std::nullopt
    ) noexcept;

    Position(Position const&) = delete;
    Position& operator=(Position const&) = delete;
    AGIS_API [[nodiscard]] auto const& trades() noexcept { return this->_trades; }
    AGIS_API [[nodiscard]] double get_realized_pnl() const noexcept { return this->_realized_pnl; }
    AGIS_API [[nodiscard]] double get_unrealized_pnl() const noexcept { return this->_unrealized_pnl; }
    AGIS_API [[nodiscard]] double get_avg_price() const { return this->_avg_price; }
    AGIS_API [[nodiscard]] double get_nlv() const { return this->_nlv; }
    AGIS_API [[nodiscard]] size_t get_asset_index() const { return this->_asset_index; }
    AGIS_API [[nodiscard]] double get_unrealized_pl() const { return this->_unrealized_pnl; }
    AGIS_API [[nodiscard]] double get_realized_pl() const { return this->_realized_pnl; }
    AGIS_API [[nodiscard]] double get_units() const { return this->_units; }
    AGIS_API [[nodiscard]] bool trade_exists(size_t strategy_index) const noexcept;
    AGIS_API [[nodiscard]] std::optional<Trade const*> get_trade(size_t strategy_index) const noexcept;

};

}