module;

#include "AgisDeclare.h"
#include "AgisMacros.h"
#include "AgisAST.h"

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
	_exchange_node(std::move(exchange_node)),
	_asset_lambda(std::move(_assetLambdaNode)),
	_exchange_view(_exchange_node->evaluate()),
	_exchange(_exchange_node->evaluate())
{
	auto& assets = _exchange->get_assets();
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
			_exchange_view.remove_allocation(view_index);
			view_index++;
			continue;
		}
		// execute asset lambda on the given asset
		auto res = _asset_lambda->evaluate(asset);
		if(!res) return AgisException("Unexpected failure to execute Asset Lambda Node");
		
		// disable asset if nan
		if (std::isnan(res.value())) {
			_exchange_view.remove_allocation(view_index);
			view_index++;
			continue;
		}
		_exchange_view.set_allocation(view_index, res.value());
		view_index++;
	}
	return std::nullopt;
}

}

}