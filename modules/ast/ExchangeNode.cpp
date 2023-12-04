module;
#include <cmath>
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
	UniquePtr<AssetLambdaNode> _assetLambdaNode
) :
	ExpressionNode(NodeType::ExchangeView),
	_exchange_node(exchange_node),
	_asset_lambda(std::move(_assetLambdaNode)),
	_exchange(exchange_node->evaluate())
{
	auto& assets = _exchange->get_assets();
	_exchange_view.resize(assets.size());
	_exchange_view.setZero();

	for (auto& asset : assets)
	{
		_assets.push_back(asset.get());
	}
	_warmup = _asset_lambda->get_warmup();
	_exchange_offset = _exchange->get_index_offset();
}


//==================================================================================================
ExchangeViewNode::~ExchangeViewNode()
{
}


//==================================================================================================
std::optional<UniquePtr<AssetLambdaReadNode>>
	ExchangeNode::create_asset_lambda_read_node(std::string const& column, int index) const noexcept
{
	auto index_res = _exchange->get_column_index(column);
	if (!index_res) return std::nullopt;
	return std::make_unique<AssetLambdaReadNode>(index_res.value(), index);
}


//==================================================================================================
std::expected<Eigen::VectorXd const*, AgisException>
ExchangeViewNode::evaluate() noexcept
{
	for (auto const asset : _assets)
	{
		auto view_index = asset->get_index() - _exchange_offset;
		if (
			!asset->is_streaming()
			||
			asset->get_current_index() < _warmup
			)
		{
			_exchange_view[view_index] = std::numeric_limits<double>::quiet_NaN();
			continue;
		}
		// execute asset lambda on the given asset
		auto res = _asset_lambda->evaluate(asset);
		if (!res) return std::unexpected<AgisException>("Unexpected failure to execute Asset Lambda Node");
		// disable asset if nan
		if (std::isnan(res.value()))
		{
			_exchange_view[view_index] = 0.0f;
		}
		else
		{
			_exchange_view[view_index] = *res;
		}
	}
	return &_exchange_view;
}


//==================================================================================================
std::unordered_map<std::string, ExchangeQueryType> const&
ExchangeViewSortNode::ExchangeQueryTypeMap()
{
	static std::unordered_map<std::string, ExchangeQueryType> const map = {
		{ "Default", ExchangeQueryType::Default },
		{ "NSmallest", ExchangeQueryType::NSmallest },
		{ "NLargest", ExchangeQueryType::NLargest },
		{ "NExtreme", ExchangeQueryType::NExtreme }
	};
	return map;
}

//==================================================================================================
void
	ExchangeViewSortNode::sort(size_t N, ExchangeQueryType sort_type) {
	if (_view.size() <= N) {
		return;
	}

	switch (sort_type) {
	case ExchangeQueryType::Default:
		_view.resize(N);  // Resize the vector to keep only the first N elements
		return;
	case ExchangeQueryType::NSmallest:
		std::partial_sort(_view.begin(), _view.begin() + N, _view.end(),
			[](const auto& a, const auto& b) {
				return a.second < b.second;
			});
		_view.resize(N);
		return;
	case ExchangeQueryType::NLargest:
		std::partial_sort(_view.begin(), _view.begin() + N, _view.end(),
			[](const auto& a, const auto& b) {
				return a.second > b.second;
			});
		_view.resize(N);
		return;
	case ExchangeQueryType::NExtreme: {
		auto n = N / 2;
		std::partial_sort(_view.begin(), _view.begin() + n, _view.end(),
			[](const auto& a, const auto& b) {
				return a.second > b.second;
			});
		std::partial_sort(_view.begin() + n, _view.begin() + N, _view.end(),
			[](const auto& a, const auto& b) {
				return a.second < b.second;
			});
		_view.resize(N);  // Resize the vector to keep only the first N elements
		return;
	}
	}
}


//==================================================================================================
ExchangeViewSortNode::~ExchangeViewSortNode()
{
}


//==================================================================================================
std::expected<Eigen::VectorXd*, AgisException>
ExchangeViewSortNode::evaluate() noexcept
{
	auto error_opt = _exchange_view_node->evaluate();
	if (!error_opt.has_value()) return std::unexpected<AgisException>(error_opt.error());
	auto const& exchange_view = *(error_opt.value());

	// copy index and element of views into sorted_view
	_view.clear();
	for (int i = 0; i < exchange_view.size(); i++) {
		if (std::isnan(exchange_view[i])) {
			continue;
		}
		_view.push_back(std::make_pair(i, exchange_view[i]));
	}
	this->sort(_N, _query_type);
	// fill weights vector with nan, and set actual allocations to the sorted view values
	_weights.setConstant(std::numeric_limits<double>::quiet_NaN());
	for (auto& pair : _view) {
		_weights[pair.first] = pair.second;
	}
	return &_weights;

}

}

}