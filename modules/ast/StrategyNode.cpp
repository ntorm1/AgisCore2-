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
StrategyNode::StrategyNode(Agis::Strategy& strategy, UniquePtr<AllocationNode> alloc_node, double epsilon)
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

	if (_epsilon < 0)
	{
		// For all weights that have the same sign as current weights, set the
		// weight to the value in current weights (i.e. do nothing if position side has not changed)
		weights = (current_weights.array().sign() == weights.array().sign()).select(current_weights, weights);
	}

	// find the adjustments need
	weights -= current_weights;

	// Set weights to 0 where the absolute difference is less than abs(_epsilon)
	weights = (weights.array().abs() < abs(_epsilon)).select(0.0, weights);
	return _strategy.set_allocation(weights);
}


}

}