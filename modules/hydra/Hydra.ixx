module;

#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif

#include "AgisDeclare.h"

export module HydraModule;

import <string>;
import <vector>;
import <memory>;
import <shared_mutex>;
import <unordered_map>;

import AgisTypes;
import AgisError;

namespace Agis
{

struct HydraPrivate;

export enum class HydraState : uint8_t

{
	INIT,
	BUILT,
	RUN,
	STOP,
	FINISHED,
};

export class Hydra
{
private:
	HydraPrivate* _p;
	HydraState _state = HydraState::INIT;
	std::atomic<bool> _running = false;
	mutable std::shared_mutex _mutex;

public:
	AGIS_API Hydra();
	AGIS_API ~Hydra();

	AGIS_API std::unique_lock<std::shared_mutex> __aquire_write_lock() const noexcept;
	AGIS_API std::shared_lock<std::shared_mutex> __aquire_read_lock() const noexcept;
	AGIS_API void interupt() noexcept { _running.store(false);}
	AGIS_API [[nodiscard]] HydraState get_state() const noexcept { return _state; }
	AGIS_API [[nodiscard]] long long get_next_global_time() const noexcept;
	AGIS_API [[nodiscard]] long long get_global_time() const noexcept;
	AGIS_API [[nodiscard]] size_t get_current_index() const noexcept;
	AGIS_API [[nodiscard]] Result<bool, AgisException> run() noexcept;
	AGIS_API [[nodiscard]] Result<bool, AgisException> run_to(long long dt) noexcept;
	AGIS_API [[nodiscard]] Result<bool, AgisException> build() noexcept;
	AGIS_API [[nodiscard]] Result<bool, AgisException> step() noexcept;
	AGIS_API [[nodiscard]] Result<bool, AgisException> reset() noexcept;
	AGIS_API [[nodiscard]] std::unordered_map<std::string, Strategy*> const& get_strategies() const noexcept;
	AGIS_API [[nodiscard]] Optional<Strategy const*> get_strategy(std::string const& strategy_id) const noexcept;
	AGIS_API [[nodiscard]] Optional<Strategy*> get_strategy_mut(std::string const& strategy_id) const noexcept;
	AGIS_API [[nodiscard]] Optional<Portfolio const*> get_portfolio(std::string const& portfolio_id) const noexcept;
	AGIS_API [[nodiscard]] Optional<Portfolio*> get_portfolio_mut(std::string const& portfolio_id) const noexcept;
	

	AGIS_API [[nodiscard]] ExchangeMap const& get_exchanges() const noexcept;
	AGIS_API [[nodiscard]] Optional<Exchange const*> get_exchange(std::string const& exchange_id) const noexcept;
	AGIS_API [[nodiscard]] Optional<Exchange*> get_exchange_mut(std::string const& exchange_id) const noexcept;
	AGIS_API [[nodiscard]] std::vector<long long> const& get_dt_index() const noexcept;
	AGIS_API [[nodiscard]] bool asset_exists(std::string const& asset_id) const noexcept;
	AGIS_API [[nodiscard]] Result<bool, AgisException> register_strategy(std::unique_ptr<Strategy> strategy);
	AGIS_API [[nodiscard]] Result<bool, AgisException> remove_strategy(Strategy& strategy);
	AGIS_API [[nodiscard]] Result<Portfolio*, AgisException> create_portfolio(
		std::string portfolio_id,
		Optional<std::string> exchange_id = std::nullopt, 
		Optional<Portfolio*> parent = std::nullopt,
		double cash = 0.0f
	);
	AGIS_API [[nodiscard]] Result<Exchange const*, AgisException> create_exchange(
		std::string exchange_id,
		std::string dt_format,
		std::string source,
		Optional<std::vector<std::string>> symbols = std::nullopt
	);
};

}