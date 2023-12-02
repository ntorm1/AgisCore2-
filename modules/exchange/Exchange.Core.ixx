module;
#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif
#include "AgisDeclare.h"
#include <ankerl/unordered_dense.h>

export module ExchangeModule;

import <string>;
import <expected>;
import <optional>;
import <vector>;
import <shared_mutex>;

import AgisError;

namespace Agis
{

struct ExchangePrivate;

export class Exchange
{
	friend class ExchangeFactory;
	friend class ExchangeMap;
	friend class Portfolio;
private:
	std::string _source;
	ExchangePrivate* _p;
	size_t _index_offset = 0;
	std::optional<std::vector<std::string>> _symbols;
	/// <summary>
	/// Mapping between portfolio child index and child portfolio
	/// </summary>
	ankerl::unordered_dense::map<size_t, Portfolio*> registered_portfolios;

	static UniquePtr<Exchange> create(
		std::string exchange_name,
		std::string dt_format,
		size_t exchange_index,
		std::string source,
		std::optional<std::vector<std::string>> symbols
	);
	static void destroy(Exchange* exchange) noexcept;
	std::expected<bool, AgisException> load_h5() noexcept;
	std::expected<bool, AgisException> load_folder() noexcept;
	std::expected<bool, AgisException> load_assets() noexcept;
	[[nodiscard]] std::expected<bool, AgisException> step(long long global_dt) noexcept;
	void register_portfolio(Portfolio* p) noexcept;
	void reset() noexcept;
	void build() noexcept;
	[[nodiscard]] std::optional<std::unique_ptr<Order>> place_order(std::unique_ptr<Order> order) noexcept;
	
	void process_market_order(Order* order) noexcept;
	void process_order(Order* order) noexcept;
	void process_orders(bool on_close) noexcept;

	bool is_valid_order(Order const* order) const noexcept;
	
	void set_index_offset(size_t offset) noexcept { _index_offset = offset;}
	std::vector<UniquePtr<Asset>>& get_assets_mut() noexcept;

public:
	Exchange(
		std::string exchange_id,
		size_t exchange_index,
		std::string dt_format,
		std::string source,
		std::optional<std::vector<std::string>> symbols
	) noexcept;

	AGIS_API ~Exchange();

	Exchange(Exchange const&) = delete;
	Exchange& operator=(Exchange const&) = delete;
	size_t get_exchange_index() const noexcept;
	std::optional<size_t> get_column_index(std::string const& column) const noexcept;
	long long get_dt() const noexcept;
	std::string const& get_exchange_id() const noexcept;
	std::string const& get_dt_format() const noexcept;
	std::string const& get_source() const noexcept{ return _source; }
	std::optional<double> get_market_price(size_t asset_index) const noexcept;
	std::optional<size_t> get_asset_index(std::string const& asset_id) const noexcept;
	std::vector<long long> const& get_dt_index() const noexcept;
	size_t get_index_offset() const noexcept { return _index_offset; }
	
	AGIS_API std::optional<double> get_covariance(size_t index1, size_t index2) const noexcept;
	AGIS_API std::expected<bool, AgisException> init_covariance_matrix(size_t lookback, size_t step_size) noexcept;
	AGIS_API std::vector<UniquePtr<Asset>> const& get_assets() const noexcept;
	AGIS_API std::optional<Asset const*> get_asset(size_t asset_index) const noexcept;
	AGIS_API std::optional<Asset const*> get_asset(std::string const& asset_id) const noexcept;
	AGIS_API std::vector<std::string> const& get_columns() const noexcept;
};



}