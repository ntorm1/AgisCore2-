module;

#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif

#include "AgisDeclare.h"

export module HydraModule;

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

	AGIS_API std::expected<bool, AgisException> build() noexcept;

	[[nodiscard]] ExchangeMap const& get_exchanges() const noexcept;
	AGIS_API [[nodiscard]] std::vector<long long> const& get_dt_index() const noexcept;

	AGIS_API [[nodiscard]] bool asset_exists(std::string const& asset_id) const noexcept;

	AGIS_API [[nodiscard]] std::expected<Exchange const*, AgisException> create_exchange(
		std::string exchange_id,
		std::string dt_format,
		std::string source
	);



};

}