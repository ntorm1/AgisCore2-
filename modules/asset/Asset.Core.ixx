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

	size_t get_index() const noexcept { return _asset_index;}
	size_t rows() const noexcept;
	size_t columns() const noexcept;
	AssetState get_state() const noexcept { return _state;}
	std::optional<double> get_market_price(bool is_close) const noexcept;
	std::optional<double> get_asset_feature(size_t column, int index) const noexcept;
	std::string const& get_dt_format() const noexcept { return _dt_format; }
	size_t get_current_index() const noexcept;
	inline bool is_streaming() const noexcept 
	{
		return _state == AssetState::STREAMING || _state == AssetState::LAST;
	}
	AGIS_API std::optional<std::vector<double>> get_column(std::string const& column_name) const noexcept;
	AGIS_API std::optional<size_t> get_streaming_index() const noexcept;
	AGIS_API std::vector<long long> const& get_dt_index() const noexcept;
	AGIS_API std::vector<std::string> get_column_names() const noexcept;
	AGIS_API std::vector<double> const& get_data() const noexcept;
	AGIS_API std::string const& get_close_column() const noexcept;
	AGIS_API std::string const& get_id() const noexcept { return _asset_id; }


	// delete copy constructor and assignment operator
	Asset(const Asset& other) = delete;
	Asset& operator=(const Asset& other) = delete;
	Asset(AssetPrivate* asset, std::string asset_id, size_t asset_index);
private:

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

	std::expected<UniquePtr<Asset>, AgisException> create_asset(
		std::string asset_name,
		std::string source
	);

private:
	size_t _asset_counter = 0;
	std::string _dt_format;
	std::string _exchange_id;
};


}