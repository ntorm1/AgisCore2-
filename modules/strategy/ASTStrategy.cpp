module;
#include "AgisDeclare.h"
#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
module ASTStrategyModule;

namespace Agis
{


//============================================================================
ASTStrategy::ASTStrategy(
	std::string strategy_id,
	double cash,
	Exchange const& exchange,
	Portfolio& portfolio,
	std::string graph_file_path) : Strategy(strategy_id, cash, exchange, portfolio)
{
	_graph_file_path = graph_file_path;
}

//============================================================================
std::expected<bool, AgisException> ASTStrategy::step() noexcept
{
	return true;
}


//============================================================================
void
ASTStrategy::serialize(rapidjson::Document& document, rapidjson::Document::AllocatorType& allocator) const noexcept
{
	rapidjson::Value v(_graph_file_path.c_str(), allocator);
	document.AddMember("graph_file_path", v.Move(), allocator);
}

}