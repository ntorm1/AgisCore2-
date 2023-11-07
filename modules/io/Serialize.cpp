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
std::expected<bool, AgisException> deserialize_exchange_map(rapidjson::Document const& json, Hydra* hydra)
{
	if (!json.HasMember("exchanges"))
	{
		return std::unexpected(AgisException("Json file does not have exchanges"));
	}
	auto& exchanges = json["exchanges"];
	if (!exchanges.IsObject())
	{
		return std::unexpected(AgisException("Json file does not have exchanges: "));
	}
	for (auto& exchange : exchanges.GetObject())
	{
		auto exchange_id = exchange.name.GetString();
		auto source = exchange.value["source_dir"].GetString();
		auto dt_format = exchange.value["dt_format"].GetString();
		auto res = hydra->create_exchange(exchange_id, dt_format, source);
		if (!res)
		{
			return std::unexpected(res.error());
		}
	}
	return true;
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


//============================================================================
std::expected<std::unique_ptr<Hydra>, AgisException>
deserialize_hydra(std::string const& path)
{
	// validate path exists
	if (!std::filesystem::exists(path))
	{
		return std::unexpected(AgisException("File does not exist: " + path));
	}
	// validate it is json file
	if (std::filesystem::path(path).extension() != ".json")
	{
		return std::unexpected(AgisException("File is not a json file: " + path));
	}
	// read it into rapidjson document
	std::ifstream in(path);
	std::string json_str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	rapidjson::Document doc;
	doc.Parse(json_str.c_str());
	if (doc.HasParseError())
	{
		return std::unexpected(AgisException("Failed to parse json file: " + path));
	}

	std::unique_ptr<Hydra> hydra = std::make_unique<Hydra>();
	AGIS_ASSIGN_OR_RETURN(res, deserialize_exchange_map(doc, hydra.get()));

	return std::move(hydra);
}



}