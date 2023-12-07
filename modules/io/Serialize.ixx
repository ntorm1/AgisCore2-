module;
#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif

#include "AgisDeclare.h"

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

export module SerializeModule;

import <expected>;
import <optional>;
import <string>;

import AgisError;

//using JsonAllocator = rapidjson::Document::AllocatorType;
// using JsonDocument = rapidjson::Document;

namespace Agis
{

typedef std::expected<rapidjson::Value, AgisException> SerializeResult;


//============================================================================
export AGIS_API SerializeResult serialize_hydra(
	rapidjson::Document::AllocatorType& allocator,
	Hydra const& hydra,
	std::optional<std::string> out_path = std::nullopt
);

export AGIS_API std::expected<std::unique_ptr<Hydra>, AgisException> deserialize_hydra(
	std::string const& path
);

}

module:private;

namespace Agis
{

//============================================================================
std::expected<bool, AgisException> deserialize_exchange_map(
	rapidjson::Document const& json,
	Hydra* hydra
);


//============================================================================
rapidjson::Document serialize_exchange_map(
	rapidjson::Document::AllocatorType& allocator,
	ExchangeMap const& exchange_map
);


//============================================================================
rapidjson::Document serialize_exchange(
	rapidjson::Document::AllocatorType& allocator,
	Exchange const& exchange
);


//============================================================================
rapidjson::Document serialize_portfolio(
	rapidjson::Document::AllocatorType& allocator,
	Portfolio const& portfolio
);

//============================================================================
std::expected<bool, AgisException> deserialize_exchange_map(
	rapidjson::Document const& json,
	Hydra* hydra
);



//============================================================================
std::expected<UniquePtr<Strategy>, AgisException> deserialize_strategy(
	rapidjson::Value const& json,
	Hydra* hydra
);

}
