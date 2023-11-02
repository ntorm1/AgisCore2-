module;

#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif

#include "AgisDeclare.h"

export module ExchangeMapModule;

import <unordered_map>;
import <string>;
import <vector>;
import <expected>;
import <optional>;

import AgisError;

namespace Agis
{


struct ExchangeMapPrivate;

//============================================================================
export class ExchangeFactory
{
	friend class ExchangeMap;
private:
	size_t _exchange_counter = 0;

	[[nodiscard]] std::expected<Exchange*, AgisException> create_exchange(
		std::string exchange_id,
		std::string dt_format,
		std::string source
	);


};

export class ExchangeMap
{
	friend class Hydra;
private:
	ExchangeMapPrivate* _p;

	void build() noexcept;

	[[nodiscard]] std::expected<Exchange const*, AgisException> create_exchange(
		std::string exchange_id,
		std::string dt_format,
		std::string source
	);

public:
	AGIS_API ExchangeMap();
	AGIS_API ~ExchangeMap();

	AGIS_API bool asset_exists(std::string const& id) const noexcept;
	AGIS_API std::expected<Asset const*, AgisException> get_asset(std::string const& id) const noexcept;
	AGIS_API std::expected<Exchange const*, AgisException> get_exchange(std::string const& id) const noexcept;

};





}