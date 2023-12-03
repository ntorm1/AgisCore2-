module;
#include <cmath>
#include <unordered_map>
#include "AgisDeclare.h"

module AssetNode;

import <optional>;

import AssetModule;
import AssetObserverModule;

#define AGIS_NAN std::numeric_limits<double>::quiet_NaN()


namespace Agis
{

namespace AST
{





//==================================================================================================
std::unordered_map<std::string, AgisOperator> const&
AssetLambdaNode::AgisOperatorMap()
{
	static std::unordered_map<std::string, AgisOperator> agis_opp_map = {
	{"init", AgisOperator::INIT},
	{"identity", AgisOperator::IDENTITY},
	{"add", AgisOperator::ADD},
	{"subtract", AgisOperator::SUBTRACT},
	{"multiply", AgisOperator::MULTIPLY},
	{"divide", AgisOperator::DIVIDE}
	};
	return agis_opp_map;
}

//==================================================================================================
AssetObserverNode::AssetObserverNode(AssetObserver* observer) 
	: ExpressionNode(NodeType::AssetObserver), _observer(observer)
{
}


//==================================================================================================
double
AssetObserverNode::evaluate() noexcept
{
	return _observer->value();
}


//============================================================================
std::optional<double>
AssetLambdaReadNode::evaluate(Asset const* asset) const noexcept
{
	return asset->get_asset_feature(_column, _index);
}


//============================================================================
double
AssetOpperationNode::execute_opperation(double left, double right) const noexcept
{
	switch (_opp) {
	case AgisOperator::INIT:
		return left;
	case AgisOperator::IDENTITY:
		return right;
	case AgisOperator::ADD:
		return right + left;
	case AgisOperator::SUBTRACT:
		return right - left;
	case AgisOperator::MULTIPLY:
		return right * left;
	case AgisOperator::DIVIDE:
		return right / left;
	default:
		return AGIS_NAN;
	}
}


//============================================================================
std::optional<double>
AssetOpperationNode::evaluate(Asset const* asset) const noexcept
{
	// evaluate right node which could be nullopt or nan
	auto right_res = _right_node->evaluate(asset);
	if(!right_res || std::isnan(*right_res))
		return right_res;

	// if no left node return opp applied to right result
	if (!_left_node)
		return execute_opperation(0.0f,*right_res);

	// execute left node and return opp applied to left and right result
	auto left_res = _left_node.value()->evaluate(asset);
	if(!left_res || std::isnan(*left_res))
		return left_res;
	return execute_opperation(*left_res, *right_res);
}


//============================================================================
AssetLambdaLogicalNode::AssetLambdaLogicalNode(
	UniquePtr<AssetLambdaNode> left_node,
	AgisLogicalOperator _opp,
	AgisLogicalRightVal right_node_,
	bool numeric_cast
) noexcept :	AssetLambdaNode(NodeType::AssetLogical),
	_left_node(std::move(left_node)),
	_opp(_opp),
	_right_node(std::move(right_node_)),
	_numeric_cast(numeric_cast)
{
	// set the warmup equal to the left node warmup and optionally the max of that and the right node
	set_warmup(_left_node->get_warmup());
	if (std::holds_alternative<UniquePtr<AssetLambdaNode>>(_right_node)) {
		auto& right_val_node = std::get<UniquePtr<AssetLambdaNode>>(_right_node);
		set_warmup(std::max(this->get_warmup(), right_val_node->get_warmup()));
	}
}


//============================================================================
bool
AssetLambdaLogicalNode::execute_opperation(double left, double right) const noexcept
{
	switch (_opp) {
		case AgisLogicalOperator::GREATER_THAN:
			return left > right;
		case AgisLogicalOperator::GREATER_THAN_OR_EQUAL:
			return left >= right;
		case AgisLogicalOperator::LESS_THAN:
			return left < right;
		case AgisLogicalOperator::LESS_THAN_OR_EQUAL:
			return left <= right;
		case AgisLogicalOperator::EQUAL:
			return left == right;
		case AgisLogicalOperator::NOT_EQUAL:
			return left != right;
	}
	return false;
}


//============================================================================
std::optional<double>
AssetLambdaLogicalNode::evaluate(Asset const* asset) const noexcept
{
	// execute left node to get value
	auto res = _left_node->evaluate(asset);
	bool res_bool = false;
	if (!res || std::isnan(*res))
		return res;

	// pass the result of the left node to the logical opperation with the right node
	// that is either a scaler double or another asset lambda node
	if (std::holds_alternative<double>(_right_node)) {
		auto right_val_double = std::get<double>(_right_node);
		res_bool = execute_opperation(res.value(), right_val_double);
		if (!res_bool && !_numeric_cast) res = AGIS_NAN;
	}
	else {
		auto& right_val_node = std::get<UniquePtr<AssetLambdaNode>>(_right_node);
		auto right_res = right_val_node->evaluate(asset);
		if (!right_res.has_value() || std::isnan(res.value())) return right_res;
		res_bool = execute_opperation(res.value(), right_res.value());
		if (!res_bool && !_numeric_cast) res = AGIS_NAN;
	}

	// if numeric cast, take the boolean result of the logical opperation and cast to double
	// i.e. if the logical opperation is true, return 1.0, else return 0.0
	if (_numeric_cast)	res = static_cast<double>(res_bool);
	return res;

}

}

}