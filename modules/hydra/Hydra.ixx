module;

#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif

#include "AgisDeclare.h"

export module HydraModule;

import <optional>;
import <expected>;
import <string>;
import <vector>;
import <memory>;
import <shared_mutex>;

import AgisError;

namespace Agis
{

struct HydraPrivate;

export enum class HydraState : uint8_t

{
	INIT,
	BUILT,
	RUN,
	STOP
};

export class Hydra
{
private:
	HydraPrivate* _p;
	HydraState _state = HydraState::INIT;
	mutable std::shared_mutex _mutex;

public:
	AGIS_API Hydra();
	AGIS_API ~Hydra();

	AGIS_API auto __aquire_write_lock() const noexcept
	{
		return std::unique_lock<std::shared_mutex>{ _mutex };
	}
	
	AGIS_API [[nodiscard]] HydraState get_state() const noexcept { return _state; }
	AGIS_API [[nodiscard]] long long get_next_global_time() const noexcept;
	AGIS_API [[nodiscard]] long long get_global_time() const noexcept;

	AGIS_API [[nodiscard]] std::expected<bool, AgisException> run() noexcept;
	AGIS_API [[nodiscard]] std::expected<bool, AgisException> run_to(long long dt) noexcept;
	AGIS_API [[nodiscard]] std::expected<bool, AgisException> build() noexcept;
	AGIS_API [[nodiscard]] std::expected<bool, AgisException> step() noexcept;
	AGIS_API [[nodiscard]] std::expected<bool, AgisException> reset() noexcept;
	AGIS_API [[nodiscard]] std::optional<Strategy*> get_strategy_mut(std::string const& strategy_id) const noexcept;
	AGIS_API [[nodiscard]] std::optional<Portfolio const*> get_portfolio(std::string const& portfolio_id) const noexcept;
	AGIS_API [[nodiscard]] ExchangeMap const& get_exchanges() const noexcept;
	AGIS_API [[nodiscard]] std::optional<Exchange const*> get_exchange(std::string const& exchange_id) const noexcept;
	AGIS_API [[nodiscard]] std::vector<long long> const& get_dt_index() const noexcept;

	AGIS_API [[nodiscard]] bool asset_exists(std::string const& asset_id) const noexcept;

	AGIS_API [[nodiscard]] std::expected<bool, AgisException> register_strategy(
		std::unique_ptr<Strategy> strategy
	);
	AGIS_API [[nodiscard]] std::expected<Portfolio*, AgisException> create_portfolio(
		std::string portfolio_id,
		std::optional<std::string> exchange_id = std::nullopt, 
		std::optional<Portfolio*> parent = std::nullopt,
		double cash = 0.0f
	);
	AGIS_API [[nodiscard]] std::expected<Exchange const*, AgisException> create_exchange(
		std::string exchange_id,
		std::string dt_format,
		std::string source
	);


	

};

}