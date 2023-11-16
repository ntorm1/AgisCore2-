module;

#include "AgisDeclare.h"
#include "AgisMacros.h"
#include "AgisAST.h"
#include <Eigen/Dense>

module ExchangeNode;


import AssetModule;
import ExchangeModule;
import AssetNode;

namespace Agis
{

namespace AST
{

	//==================================================================================================
ExchangeViewNode::ExchangeViewNode(
	SharedPtr<ExchangeNode const> exchange_node,
	UniquePtr<AssetLambdaNode> _assetLambdaNode):
	_exchange_node(exchange_node),
	_asset_lambda(std::move(_assetLambdaNode)),
	_exchange(exchange_node->evaluate())
{
	auto& assets = _exchange->get_assets();
	_exchange_view.resize(assets.size());
	for (auto& asset : assets)
	{
		_assets.push_back(asset.get());
	}
	_warmup = _asset_lambda->get_warmup();
	_exchange_offset = _exchange->get_index_offset();
}


//==================================================================================================
ExchangeNode::~ExchangeNode()
{
}


//==================================================================================================
std::optional<UniquePtr<AssetLambdaReadNode>>
ExchangeNode::create_asset_lambda_read_node(std::string const& column, int index) const noexcept
{
	auto index_res = _exchange->get_column_index(column);
	if(!index_res) return std::nullopt;
	return std::make_unique<AssetLambdaReadNode>(index_res.value(), index);
}


//==================================================================================================
std::optional<AgisException>
ExchangeViewNode::evaluate() noexcept
{
	size_t view_index = 0;
	for (auto const asset : _assets)
	{
		// check if asset is streaming and if it is warmup
		if (
			!asset->is_streaming()
			||
			asset->get_current_index() < _warmup
			)
		{
			_exchange_view[view_index] = 0.0f;
		}
		// execute asset lambda on the given asset
		auto res = _asset_lambda->evaluate(asset);
		if(!res) return AgisException("Unexpected failure to execute Asset Lambda Node");
		// disable asset if nan
		if (std::isnan(res.value())) 
		{
			_exchange_view[view_index] = 0.0f;
		}
		else 
		{
			_exchange_view[view_index] = res.value_or(0.0f);
		}
		view_index++;
	}
	return std::nullopt;
}

/*
//============================================================================
void
ExchangeViewSortNode::set_weights(ExchangeViewOpp opp) noexcept
{
	switch (opp)
	{
	case ExchangeViewOpp::UNIFORM:
		uniform_weights();
		break;
	}
}

//============================================================================
void ExchangeViewSortNode::uniform_weights()
{
	auto weight = 1.0f / static_cast<double>(_allocation_count);
	for (auto& pair : _view) {
		if (!pair) continue;
		pair->amount = weight;
	}
}
*/


//==================================================================================================
void
ExchangeViewSortNode::sort(Eigen::VectorXd& _view, size_t N, ExchangeQueryType sort_type) {
	if (_view.size() <= N) {
		return;
	}

	switch (sort_type) {
	case ExchangeQueryType::Default:
		_view.conservativeResize(N);  // Resize the vector to keep only the first N elements
		return;
	case ExchangeQueryType::NSmallest:
		std::partial_sort(_view.data(), _view.data() + N, _view.data() + _view.size(),
			[](const auto& a, const auto& b) {
				return a < b;
			});
		_view.conservativeResize(N);
		return;
	case ExchangeQueryType::NLargest:
		std::partial_sort(_view.data(), _view.data() + N, _view.data() + _view.size(),
			[](const auto& a, const auto& b) {
				return a > b;
			});
		_view.conservativeResize(N);
		return;
	case ExchangeQueryType::NExtreme: {
		auto n = N / 2;
		std::partial_sort(_view.data(), _view.data() + n, _view.data() + _view.size(),
			[](const auto& a, const auto& b) {
				return a > b;
			});
		std::partial_sort(_view.data() + n, _view.data() + N, _view.data() + _view.size(),
			[](const auto& a, const auto& b) {
				return a < b;
			});
		_view.conservativeResize(N);  // Resize the vector to keep only the first N elements
		return;
	}
	}
}


//==================================================================================================
std::expected<Eigen::VectorXd, AgisException>
ExchangeViewSortNode::evaluate() noexcept
{
	auto error_opt = _exchange_view_node->evaluate();
	if (error_opt) return std::unexpected<AgisException>(error_opt.value());
	Eigen::VectorXd view = _exchange_view_node->get_view();
	this->sort(view, _N, _query_type);
	return view;
}

}

}