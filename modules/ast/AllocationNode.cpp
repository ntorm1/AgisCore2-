module;

#include <Eigen/Dense>
#include "AgisDeclare.h"
#include "AgisAST.h"

module AllocationNode;
import ExchangeNode;
import AssetNode;

namespace Agis
{

namespace AST
{

//==================================================================================================
AllocationNode::AllocationNode(UniquePtr<ExchangeViewSortNode> weights_node, double epsilon, AllocType alloc_type)
{
	_weights_node = std::move(weights_node);
	_epsilon = epsilon;
	_alloc_type = alloc_type;
}


//==================================================================================================
AllocationNode::~AllocationNode()
{
}


//==================================================================================================
std::expected<Eigen::VectorXd const*, AgisException>
AllocationNode::evaluate() noexcept
{
	auto weights_opt = _weights_node->evaluate();
	if (!weights_opt)
		return std::unexpected<AgisException>(weights_opt.error());
	auto& weights = *weights_opt.value();
	set_weights(weights);
	return *weights_opt;
}


//==================================================================================================
void
AllocationNode::uniform_allocation(Eigen::VectorXd& weights)
{
	auto c = 1.0f / static_cast<double>(_weights_node->view_size());
	weights.setOnes();
	weights *= c;
}


//==================================================================================================
void AllocationNode::set_weights(Eigen::VectorXd& weights)
{
	switch (_alloc_type)
	{
		case AllocType::UNIFORM:
			return uniform_allocation(weights);
	}
}

}

}