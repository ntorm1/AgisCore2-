module;

#include "AgisDeclare.h"
#include "AgisMacros.h"

#include <fstream>

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

module SerializeModule;

import <filesystem>;

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
	
	rapidjson::Value v_id(exchange_id.c_str(), allocator);
	j.AddMember("exchange_id", v_id.Move(), allocator);
	
	rapidjson::Value v_source(source.c_str(), allocator);
	j.AddMember("source_dir", v_source.Move(), allocator);
	
	rapidjson::Value v_dt_format(dt_format.c_str(), allocator);
	j.AddMember("dt_format", v_dt_format.Move(), allocator);
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
SerializeResult serialize_hydra(
	rapidjson::Document::AllocatorType& allocator,
	Hydra const& hydra,
	std::optional<std::string> out_path)
{
	rapidjson::Value j(rapidjson::kObjectType);
	auto exchange_map_json = serialize_exchange_map(allocator, hydra.get_exchanges());
	j.AddMember("exchanges", std::move(exchange_map_json), allocator);
	
	if (out_path)
	{	
		try
		{
			rapidjson::StringBuffer buffer;
			rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer); // Use PrettyWriter for indentation
			writer.SetIndent(' ', 4);
			j.Accept(writer);
			std::ofstream out(*out_path);
			out << buffer.GetString();
			out.close();
		}
		catch (std::exception& e)
		{
			return std::unexpected(AgisException(e.what()));
		}
	}

	return j;
}



}