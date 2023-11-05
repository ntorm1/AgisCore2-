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
	friend class Portfolio;
private:
	ExchangeMapPrivate* _p;

	std::expected<bool, AgisException> step() noexcept;
	std::expected<bool, AgisException> build() noexcept;
	void reset() noexcept;
	void process_orders(bool on_close) noexcept;
	[[nodiscard]] std::expected<Exchange const*, AgisException> create_exchange(
		std::string exchange_id,
		std::string dt_format,
		std::string source
	);

	std::expected<bool, AgisException> force_place_order(Order* order, bool is_close) noexcept;
	AGIS_API [[nodiscard]] std::expected<Exchange*, AgisException> get_exchange_mut(std::string const& id) const noexcept;

public:
	AGIS_API ExchangeMap();
	AGIS_API ~ExchangeMap();

	AGIS_API bool asset_exists(std::string const& id) const noexcept;

	AGIS_API [[nodiscard]] std::optional<double> get_market_price(std::string const& asset_id, bool close) const noexcept;
	AGIS_API [[nodiscard]] std::expected<Asset const*, AgisException> get_asset(std::string const& id) const noexcept;
	AGIS_API [[nodiscard]] std::optional<size_t> get_asset_index(std::string const& id) const noexcept;
	AGIS_API [[nodiscard]] std::expected<Exchange const*, AgisException> get_exchange(std::string const& id) const noexcept;
	AGIS_API long long get_global_time() const noexcept;
	[[nodiscard]] std::vector<long long> const& get_dt_index() const noexcept;
};





}