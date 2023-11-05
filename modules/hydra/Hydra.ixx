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

import AgisError;

namespace Agis
{

struct HydraPrivate;

export class Hydra
{
private:
	HydraPrivate* _p;

public:
	AGIS_API Hydra();
	AGIS_API ~Hydra();

	AGIS_API [[nodiscard]] std::expected<bool, AgisException> build() noexcept;
	AGIS_API [[nodiscard]] std::expected<bool, AgisException> step() noexcept;
	AGIS_API [[nodiscard]] std::expected<bool, AgisException> reset() noexcept;

	AGIS_API [[nodiscard]] std::optional<Portfolio const*> get_portfolio(std::string const& portfolio_id) const noexcept;
	AGIS_API [[nodiscard]] ExchangeMap const& get_exchanges() const noexcept;
	AGIS_API [[nodiscard]] std::vector<long long> const& get_dt_index() const noexcept;

	AGIS_API [[nodiscard]] bool asset_exists(std::string const& asset_id) const noexcept;


	AGIS_API [[nodiscard]] std::expected<Portfolio const*, AgisException> create_portfolio(
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