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
private:
	ExchangeMapPrivate* _p;
	std::vector<Exchange*> _exchanges;
	std::unordered_map<std::string, size_t> _exchange_indecies;


public:
	AGIS_API ExchangeMap();
	AGIS_API ~ExchangeMap();


	AGIS_API bool asset_exists(std::string const& id) const noexcept;
	std::expected<Asset const*, AgisException> get_asset(std::string const& id) const noexcept;
	std::expected<Exchange const*, AgisException> get_exchange(std::string const& id) const noexcept;


	AGIS_API [[nodiscard]] std::expected<Exchange*, AgisException> create_exchange(
		std::string exchange_id,
		std::string dt_format,
		std::string source
	);


};





}