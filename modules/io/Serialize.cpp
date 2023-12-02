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
import PortfolioModule;

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
serialize_portfolio(rapidjson::Document::AllocatorType& allocator, Portfolio const& portfolio)
{
	std::string portfolio_id = portfolio.get_portfolio_id();
	std::string exchange_id = "master";
	auto exchange = portfolio.get_exchange();
	if (exchange)
	{
		exchange_id = (*exchange)->get_exchange_id();
	}
	rapidjson::Document j(rapidjson::kObjectType);
	rapidjson::Value v_portfolio_id(portfolio_id.c_str(), allocator);
	j.AddMember("portfolio_id", v_portfolio_id.Move(), allocator);
	rapidjson::Value v_exchange_id(exchange_id.c_str(), allocator);
	j.AddMember("exchange_id", v_exchange_id.Move(), allocator);

	auto& child_portfolios = portfolio.child_portfolios();
	if (child_portfolios.empty())
	{
		return j;
	}
	rapidjson::Document child_portfolios_json(rapidjson::kObjectType);
	for(const auto& [index, child_portfolio] : child_portfolios)
	{
		auto child_portfolio_json = serialize_portfolio(allocator, *(child_portfolio.get()));
		rapidjson::Value key(child_portfolio->get_portfolio_id().c_str(), allocator);
		child_portfolios_json.AddMember(key.Move(), std::move(child_portfolio_json), allocator);
	}
	j.AddMember("child_portfolios", std::move(child_portfolios_json), allocator);
	return j;
}


//============================================================================
std::expected<bool, AgisException>
deserialize_portfolio(
	rapidjson::Value const& json,
	Hydra* hydra,
	std::optional<Portfolio*> parent_portfolio)
{
	// build the new portfolio instance
	if (!json.HasMember("portfolio_id"))
	{
		return std::unexpected(AgisException("Json file does not have portfolio_id"));
	}
	std::string portfolio_id = json["portfolio_id"].GetString();
	if (!json.HasMember("exchange_id"))
	{
		return std::unexpected(AgisException("Json file does not have exchange_id"));
	}
	auto exchange_id = json["exchange_id"].GetString();
	std::expected<Portfolio*, AgisException> res;
	if (portfolio_id != "master") {
		res = hydra->create_portfolio(portfolio_id, exchange_id, parent_portfolio);
		if (!res)
		{
			return std::unexpected(res.error());
		}
	}
	// if serailized portfolio has child portfolio build them and pass pointer to the 
	// previously created portfolio
	if (json.HasMember("child_portfolios"))
	{
		auto& child_portfolios = json["child_portfolios"];
		if (!child_portfolios.IsObject())
		{
			return std::unexpected(AgisException("expected child_portfolios to be object"));
		}
		for (auto& child_portfolio : child_portfolios.GetObject())
		{
			auto const& child_json = child_portfolio.value;
			auto de_res = deserialize_portfolio(child_json, hydra, res.value());
			if (!de_res)
			{
				return std::unexpected(de_res.error());
			}
		}
	}
	return true;
}


//============================================================================
std::expected<bool, AgisException>
deserialize_exchange_map(rapidjson::Document const& json, Hydra* hydra)
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
	auto master_portfolio_json = serialize_portfolio(allocator, *(hydra.get_portfolio("master").value()));
	j.AddMember("exchanges", std::move(exchange_map_json), allocator);
	j.AddMember("master_portfolio", std::move(master_portfolio_json), allocator);
	
	if (out_path)
	{	
		rapidjson::StringBuffer buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer); // Use PrettyWriter for indentation
		writer.SetIndent(' ', 4);
		j.Accept(writer);
		std::ofstream out(*out_path);
		if (!out.is_open())
		{
			return std::unexpected(AgisException("Failed to open file: " + *out_path));
		}
		out << buffer.GetString();
		out.close();
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

	// read in portfolio json
	if (!doc.HasMember("master_portfolio"))
	{
		return std::unexpected(AgisException("Json file does not have master portfolio"));
	}
	auto const& master_portfolio_json = doc["master_portfolio"];
	AGIS_ASSIGN_OR_RETURN(res_p, deserialize_portfolio(master_portfolio_json, hydra.get(), std::nullopt));
	return std::move(hydra);
}



}