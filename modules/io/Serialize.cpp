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
import StrategyModule;
import ASTStrategyModule;
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

	// save symbol init list if it exists
	if (exchange.symbols())
	{
		auto const& symbs_vec = *exchange.symbols();
		rapidjson::Document symbols_json(rapidjson::kArrayType);
		for (auto const& symb : symbs_vec)
		{
			rapidjson::Value v_symb(symb.c_str(), allocator);
			symbols_json.PushBack(v_symb.Move(), allocator);
		}
		j.AddMember("symbols", std::move(symbols_json), allocator);
	}

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

	rapidjson::Document child_portfolios_json(rapidjson::kObjectType);
	rapidjson::Document child_strategy_json_all(rapidjson::kObjectType);
	for(const auto& [index, child_portfolio] : portfolio.child_portfolios())
	{
		auto child_portfolio_json = serialize_portfolio(allocator, *(child_portfolio.get()));
		rapidjson::Value key(child_portfolio->get_portfolio_id().c_str(), allocator);
		child_portfolios_json.AddMember(key.Move(), std::move(child_portfolio_json), allocator);
	}
	for (const auto& [index, child_strategy] : portfolio.child_strategies())
	{
		rapidjson::Document child_strategy_json(rapidjson::kObjectType);
		child_strategy->serialize_base(child_strategy_json, allocator);
		child_strategy->serialize(child_strategy_json, allocator);
		rapidjson::Value key(child_strategy->get_strategy_id().c_str(), allocator);
		child_strategy_json_all.AddMember(key.Move(), std::move(child_strategy_json), allocator);
	}
	j.AddMember("child_strategies", std::move(child_strategy_json_all), allocator);
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
	else
	{
		res = hydra->get_portfolio_mut("master").value();
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
	if (!json.HasMember("child_strategies"))
	{
		return std::unexpected(AgisException("Json file does not have child_strategies"));
	}
	auto& child_strategies = json["child_strategies"];
	for (auto& child_strategy : child_strategies.GetObject())
	{
		auto const& child_json = child_strategy.value;
		AGIS_ASSIGN_OR_RETURN(strategy, deserialize_strategy(child_json, hydra));
		AGIS_ASSIGN_OR_RETURN(v, hydra->register_strategy(std::move(strategy)));
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

		// load in init symbol list if it exists
		std::optional<std::vector<std::string>> _symbols;
		if (exchange.value.HasMember("symbols"))
		{
			auto& symbols = exchange.value["symbols"];
			if (!symbols.IsArray())
			{
				return std::unexpected(AgisException("expected symbols to be array"));
			}
			std::vector<std::string> symbols_vec;
			for (auto& symbol : symbols.GetArray())
			{
				symbols_vec.push_back(symbol.GetString());
			}
			_symbols = symbols_vec;
		}

		auto res = hydra->create_exchange(exchange_id, dt_format, source, _symbols);
		if (!res)
		{
			return std::unexpected(res.error());
		}
	}
	return true;
}


//============================================================================
std::expected<UniquePtr<Strategy>, AgisException> 
deserialize_strategy(rapidjson::Value const& json, Hydra* hydra)
{
	if (
		!json.HasMember("strategy_id") ||
		!json.HasMember("portfolio_id")||
		!json.HasMember("exchange_id") ||
		!json.HasMember("graph_file_path") ||
		!json.HasMember("starting_cash")
		)
	{
		return std::unexpected(AgisException("strategy json file missing information"));
	}
	AGIS_OPTIONAL_REF(exchange, hydra->get_exchange(json["exchange_id"].GetString()));
	AGIS_OPTIONAL_REF(portfolio, hydra->get_portfolio_mut(json["portfolio_id"].GetString()));
	auto strategy_id = json["strategy_id"].GetString();
	auto graph_file_path = json["graph_file_path"].GetString();
	double starting_cash = json["starting_cash"].GetDouble();
	auto s = std::make_unique<ASTStrategy>(strategy_id, starting_cash, *exchange, *portfolio, graph_file_path);
	AGIS_ASSIGN_OR_RETURN(res, s->deserialize(json));
	return std::move(s);
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
	auto lock = hydra.__aquire_read_lock();
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