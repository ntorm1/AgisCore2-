module;
#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif

#include "AgisDeclare.h"

export module TradeModule;

import <atomic>;


namespace Agis
{

export class Trade
{
    friend class Position;
    friend class Portfolio;
private:
    static std::atomic<size_t> _trade_counter;
    Strategy*       _strategy;
    Portfolio*      _portfolio;
    Asset const&    _asset;
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

    size_t _trade_id;
    size_t _asset_index;
    size_t _strategy_index;
    size_t _portfolio_index;

    void close(Order const* filled_order);
    void increase(Order const* filled_order);
    void reduce(Order const* filled_order);
    void adjust(Order const* filled_order);
    void evaluate(double market_price, bool on_close, bool is_reprice);
    Asset const& get_asset() const { return _asset; }
    Strategy* get_strategy_mut() const { return _strategy; }

public:
    Trade(Strategy* strategy, Order const* order) noexcept;
    ~Trade() = default;
    AGIS_API size_t get_portfolio_index() const { return _portfolio_index; }
    AGIS_API size_t get_asset_index() const { return _asset_index; }
    AGIS_API size_t get_strategy_index() const { return _strategy_index; }
    AGIS_API double get_units() const { return _units; }
    AGIS_API double get_avg_price() const { return _avg_price; }
    AGIS_API double get_open_price() const { return _open_price; }
    AGIS_API double get_close_price() const { return _close_price; }
    AGIS_API double get_last_price() const { return _last_price; }
    AGIS_API double get_nlv() const { return _nlv; }
    AGIS_API double get_unrealized_pnl() const { return _unrealized_pnl; }
    AGIS_API double get_realized_pnl() const { return _realized_pnl; }
    AGIS_API long long get_open_time() const { return _open_time; }
    AGIS_API long long get_close_time() const { return _close_time; }

};



}