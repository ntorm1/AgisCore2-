module;
#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif
#include "AgisDeclare.h"
#include <Eigen/Dense>
export module StrategyTracerModule;

import <vector>;
import <atomic>;
import <optional>;
import <bitset>;
import <expected>;

import AgisError;

namespace Agis
{

export enum Tracer : size_t {
    NLV = 0,
    CASH = 1,
    UNREALIZED_PNL = 2,
    MAX = 5
};

export class StrategyTracers {
    friend class Strategy;
    friend class Portfolio;
    friend class Trade;
public:


    StrategyTracers(Strategy* strategy_, double cash, size_t asset_count, size_t exchange_offset);
    StrategyTracers(Portfolio* portfolio, double cash);

    StrategyTracers(Strategy* strategy_, std::initializer_list<Tracer> opts) {
        this->strategy = strategy_;
        for (const Tracer& opt : opts)
            value_.set(opt);
    }

    inline bool has(Tracer o) const noexcept { return value_[o]; }
    std::optional<double> get(Tracer o) const;
    void set(Tracer o) { value_.set(o); }
    void reset(Tracer o) { value_.reset(o); }

    [[nodiscard]] std::expected<bool, AgisException> evaluate();
    void zero_out_tracers();
    void reset();

    void starting_cash_add_assign(double v) noexcept;
    void cash_add_assign(double v) noexcept;
    void nlv_add_assign(double v) noexcept;
    void unrealized_pnl_add_assign(double v) noexcept;

protected:
    std::vector<double> nlv_history;
    std::vector<double> cash_history;

    std::atomic<double> nlv = 0;
    std::atomic<double> unrealized_pnl = 0;
    std::atomic<double> cash = 0;
    std::atomic<double> starting_cash = 0;
private:
    inline Eigen::VectorXd const& get_weights() const noexcept { return _weights; }
    inline void zero_allocation(size_t i) noexcept { _weights[i- _exchange_offset] = 0; }
    inline void allocation_add_assign(size_t i, double v) noexcept { _weights[i- _exchange_offset] += v; }
    Strategy* strategy = nullptr;
    Portfolio* portfolio = nullptr;
    size_t _exchange_offset = 0;
    Eigen::VectorXd _weights;
    std::bitset<MAX> value_{ 0 };
    std::vector<double> beta_history;

};


}