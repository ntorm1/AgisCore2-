module;

#include <Eigen/Dense>
#include "AgisDeclare.h"
#include "AgisAST.h"
#include "AgisMacros.h"

module StrategyNode;

import StrategyModule;
import AllocationNode;

namespace Agis
{

namespace AST
{


//==================================================================================================
StrategyNode::StrategyNode(Strategy& strategy, UniquePtr<AllocationNode> alloc_node, double epsilon)
	:	ExpressionNode(NodeType::Strategy),
		_strategy(strategy),
		_alloc_node(std::move(alloc_node)),
		_epsilon(epsilon)
{
}


//==================================================================================================
StrategyNode::~StrategyNode()
{
}


//==================================================================================================
std::expected<bool, AgisException>
StrategyNode::evaluate() noexcept
{
	AGIS_ASSIGN_OR_RETURN(weights_ptr, _alloc_node->evaluate());
	Eigen::VectorXd const& current_weights = _strategy.get_weights();
	Eigen::VectorXd& weights = *weights_ptr;

	weights -= current_weights;
	weights = (weights.array().abs() < _epsilon).select(0.0, weights);
	return _strategy.set_allocation(weights);
}


}

}