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
AllocationNode::AllocationNode(
	UniquePtr<ExchangeViewSortNode> weights_node,
	AllocType alloc_type,
	std::optional<AllocParams> alloc_params
	): ExpressionNode(NodeType::Allocation)
{
	_weights_node = std::move(weights_node);
	_alloc_type = alloc_type;
	if (alloc_params)
		_alloc_params = *alloc_params;
	else
		_alloc_params = AllocParams();
}


//==================================================================================================
AllocationNode::~AllocationNode()
{
}


//==================================================================================================
std::unordered_map<std::string, AllocType> const&
AllocationNode::AllocTypeMap()
{
	static std::unordered_map<std::string, AllocType> alloc_type_map = {
		{"uniform", AllocType::UNIFORM},
		{"rank", AllocType::RANK}
	};
	return alloc_type_map;
}


//==================================================================================================
AGIS_API std::optional<std::string> AllocationNode::AllocTypeToString(AllocType type)
{
	if (type == AllocType::UNIFORM)
		return "uniform";
	if (type == AllocType::RANK)
		return "rank";
	return std::nullopt;
}


//==================================================================================================
std::expected<Eigen::VectorXd*, AgisException>
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
AllocationNode::rank_allocation(Eigen::VectorXd& weights)
{
	std::vector<int> indices(weights.size(), 0);
	std::sort(indices.begin(), indices.end(), [&weights](int i1, int i2) {
		return weights[i1] > weights[i2];
	});
	std::vector<double> ranks(weights.size(), 0.0f);
	double size = static_cast<double>(weights.size());
	double sum = size * (size + 1) / 2;
	for (size_t i = 0; i < indices.size(); ++i) {
		ranks[indices[i]] = (static_cast<double>(i) / sum);
	}
}


//==================================================================================================
void
AllocationNode::uniform_allocation(Eigen::VectorXd& weights)
{
	if (!_weights_node->view_size()) {
		weights.setZero();
		return;
	}
	auto c = 1.0f / static_cast<double>(_weights_node->view_size());
	if(_alloc_params.weight_clip) {
		c = std::clamp(c, -1*(*_alloc_params.weight_clip), *_alloc_params.weight_clip);
	}
	for (auto i = 0; i < weights.size(); ++i) {
		if (std::isnan(weights[i])) {
			weights[i] = 0.0f;
		}
		else {
			weights[i] = c;
		}
	}
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