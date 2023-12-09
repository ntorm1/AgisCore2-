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

	[[nodiscard]] std::expected<UniquePtr<Exchange>, AgisException> create_exchange(
		std::string exchange_id,
		std::string dt_format,
		std::string source,
		std::optional<std::vector<std::string>> symbols
	);


};

export class ExchangeMap
{
	friend class Hydra;
	friend class Portfolio;
private:
	ExchangeMapPrivate* _p;
	size_t candles = 0;
	std::expected<bool, AgisException> step() noexcept;
	std::expected<bool, AgisException> build() noexcept;
	void reset() noexcept;
	void process_orders(bool on_close) noexcept;
	[[nodiscard]] std::expected<Exchange const*, AgisException> create_exchange(
		std::string exchange_id,
		std::string dt_format,
		std::string source,
		std::optional<std::vector<std::string>> symbols
	);
	std::vector<Asset*> const& get_assets() const noexcept;
	std::expected<bool, AgisException> force_place_order(Order* order, bool is_close) noexcept;
	AGIS_API [[nodiscard]] std::expected<Exchange*, AgisException> get_exchange_mut(std::string const& id) const noexcept;

public:
	AGIS_API ExchangeMap();
	AGIS_API ~ExchangeMap();

	AGIS_API bool asset_exists(std::string const& id) const noexcept;
	AGIS_API size_t get_candle_count() const noexcept { return candles; }
	AGIS_API std::unordered_map<std::string, size_t> const& get_exchange_indecies() const noexcept;
	AGIS_API [[nodiscard]] std::optional<double> get_market_price(std::string const& asset_id, bool close) const noexcept;
	AGIS_API [[nodiscard]] std::expected<Asset const*, AgisException> get_asset(std::string const& id) const noexcept;
	AGIS_API [[nodiscard]] std::optional<size_t> get_asset_index(std::string const& id) const noexcept;
	AGIS_API [[nodiscard]] std::optional<std::string> get_asset_id(size_t index) const noexcept;
	AGIS_API [[nodiscard]] std::expected<Exchange const*, AgisException> get_exchange(std::string const& id) const noexcept;
	AGIS_API long long get_global_time() const noexcept;
	long long get_next_time() const noexcept;
	[[nodiscard]] std::vector<long long> const& get_dt_index() const noexcept;
};





}