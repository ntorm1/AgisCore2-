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

namespace Agis
{

typedef std::expected<rapidjson::Value, AgisException> SerializeResult;


//============================================================================
export AGIS_API SerializeResult serialize_hydra(
	rapidjson::Document::AllocatorType& allocator,
	Hydra const& hydra,
	std::optional<std::string> out_path = std::nullopt
);

}

module:private;

namespace Agis
{

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

}
