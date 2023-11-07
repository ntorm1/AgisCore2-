module;

#include "AgisDeclare.h"
#include "AgisMacros.h"

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

module SerializeModule;

import ExchangeModule;
import ExchangeMapModule;
import HydraModule;

namespace Agis
{

//============================================================================
rapidjson::Document
serialize_exchange(
	rapidjson::Document::AllocatorType& allocator,
	Exchange const& exchange)
{
	auto exchange_id = exchange.get_exchange_id();
	auto source = exchange.get_source();
	auto dt_format = exchange.get_dt_format();
	rapidjson::Document j(rapidjson::kObjectType);
	j.AddMember("exchange_id", rapidjson::StringRef(exchange_id.c_str()), allocator);
	j.AddMember("source_dir", rapidjson::StringRef(source.c_str()), allocator);
	j.AddMember("dt_format", rapidjson::StringRef(dt_format.c_str()), allocator);
	return j;
}


//============================================================================
rapidjson::Document
serialize_exchange_map(
	rapidjson::Document::AllocatorType& allocator,
	ExchangeMap const& exchange_map)
{
	rapidjson::Document j(rapidjson::kObjectType);
	auto exchange_ids = exchange_map.get_exchange_indecies();
	for (const auto& [id,index] : exchange_ids) {
		auto exchange = exchange_map.get_exchange(id).value();
		auto json = serialize_exchange(allocator, *exchange);
		rapidjson::Value key(id.c_str(), allocator);
		j.AddMember(key.Move(), std::move(json), allocator);
	}
	return j;
}


//============================================================================
SerializeResult serialize_hydra(rapidjson::Document::AllocatorType& allocator, Hydra const& hydra)
{
	rapidjson::Value j(rapidjson::kObjectType);
	auto exchange_map_json = serialize_exchange_map(allocator, hydra.get_exchanges());
	j.AddMember("exchanges", std::move(exchange_map_json), allocator);
	
	return j;
}



}