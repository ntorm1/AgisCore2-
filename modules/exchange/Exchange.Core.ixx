module;
#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif

#include "AgisDeclare.h"

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
private:
	std::string _source;
	ExchangePrivate* _p;
	mutable std::shared_mutex _mutex;
	size_t _index_offset = 0;

	Exchange(
		std::string exchange_id,
		size_t exchange_index,
		std::string dt_format,
		std::string source
	);

	static Exchange* create(
		std::string exchange_name,
		std::string dt_format,
		size_t exchange_index,
		std::string source
	);
	static void destroy(Exchange* exchange) noexcept;

	std::expected<bool, AgisException> load_folder() noexcept;
	std::expected<bool, AgisException> load_assets() noexcept;
	void build() noexcept;
	void set_index_offset(size_t offset) noexcept { _index_offset = offset;}
	std::vector<Asset*>& get_assets_mut() noexcept;
public:
	~Exchange();

	Exchange(Exchange const&) = delete;
	Exchange& operator=(Exchange const&) = delete;

	std::vector<Asset*> const& get_assets() const noexcept;
	AGIS_API std::optional<Asset const*> get_asset(size_t asset_index) const noexcept;

};



}