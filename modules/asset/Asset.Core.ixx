module;

#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif

#include "AgisDeclare.h"

export module AssetModule;

import <string>;
import <memory>;
import <expected>;
import <vector>;
import <optional>;

import AgisError;


namespace Agis
{

export enum AssetState: uint8_t
{
	PENDING,
	STREAMING,
	DISABLED,
	LAST,
	EXPIRED
};

class AssetPrivate;
class AssetFactory;

//============================================================================
export class Asset
{
	friend class AssetFactory;
	friend class Exchange;
public:
	~Asset();

	size_t get_index() const noexcept;
	size_t rows() const noexcept;
	size_t columns() const noexcept;
	AssetState get_state() const noexcept { return _state;}
	std::optional<double> get_market_price(bool is_close) const noexcept;
	std::string const& get_id() const noexcept { return _asset_id;}
	std::string const& get_dt_format() const noexcept { return _dt_format; }
	std::vector<long long> const& get_dt_index() const noexcept;
	std::vector<double> const& get_data() const noexcept;
	std::optional<std::vector<double>> get_column(std::string const& column_name) const noexcept;
	std::vector<std::string> get_column_names() const noexcept;

private:
	Asset(AssetPrivate* asset, std::string asset_id, size_t asset_index);

	void reset() noexcept;
	AssetState step(long long global_time) noexcept;

	size_t _asset_index;
	std::string _asset_id;
	std::string _dt_format;
	AssetState _state = AssetState::PENDING;
	AssetPrivate* _p;
};


//============================================================================
export class AssetFactory
{
public:
	AssetFactory(
		std::string dt_format,
		std::string exchange_id
	) 
	{
		_exchange_id = exchange_id;
		_dt_format = dt_format;
	}

	std::expected<Asset*, AgisException> create_asset(
		std::string asset_name,
		std::string source
	);

private:
	size_t _asset_counter = 0;
	std::string _dt_format;
	std::string _exchange_id;
};


}