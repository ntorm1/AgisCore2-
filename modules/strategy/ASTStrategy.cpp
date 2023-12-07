module;
#include "AgisMacros.h"
#include "AgisDeclare.h"
#include <Eigen/Dense>
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
	if (!_alloc_node) return true;
	AGIS_ASSIGN_OR_RETURN(weights_ptr, _alloc_node->evaluate());

	Eigen::VectorXd const& current_weights = get_weights();
	Eigen::VectorXd& weights = *weights_ptr;

	weights -= current_weights;
	weights = (weights.array().abs() < _epsilon).select(0.0, weights);
	return set_allocation(weights);
}


//============================================================================
void
ASTStrategy::serialize(rapidjson::Document& document, rapidjson::Document::AllocatorType& allocator) const noexcept
{
	rapidjson::Value v(_graph_file_path.c_str(), allocator);
	document.AddMember("graph_file_path", v.Move(), allocator);
}

}