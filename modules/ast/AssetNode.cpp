module;

#include "AgisDeclare.h"

module AssetNode;

import <optional>;

import AssetModule;


namespace Agis
{

namespace AST
{

//============================================================================
std::optional<double>
AssetLambdaReadNode::evaluate(Asset const* asset) const noexcept
{
	return asset->get_asset_feature(_column, _index);
}


//============================================================================
std::optional<double>
AssetOpperationNode::evaluate(Asset const* asset) const noexcept
{
	// evaluate right node which could be nullopt or nan
	auto res = _right_node->evaluate(asset);
	if(!res || std::isnan(*res))
		return res;

	// if no left node return opp applied to right result
	if (!_left_node)
		return _opp(0.0f,*res);

	// execute left node and return opp applied to left and right result
	auto left_res = _left_node.unwrap()->evaluate(asset);
	if(!left_res || std::isnan(*left_res))
		return left_res;
	return _opp(*left_res, *res);
}


}

}