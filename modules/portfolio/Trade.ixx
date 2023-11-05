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
private:
    static std::atomic<size_t> _trade_counter;
    Strategy*   _strategy;
    Portfolio*  _portfolio;
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

public:
    Trade(Strategy* strategy, Order const* order) noexcept;
    ~Trade() = default;
    size_t get_asset_index() const { return _asset_index; }
    double get_units() const { return _units; }

};



}