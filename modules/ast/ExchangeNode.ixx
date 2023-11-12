module;
#define NOMINMAX
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif
#include "AgisDeclare.h"
#include "AgisAST.h"
export module ExchangeNode;

import <optional>;
import <string>;
import <vector>;
import <variant>;

import BaseNode;
import AgisError;
import ExchangeViewModule;

namespace Agis
{


namespace AST
{

//==================================================================================================
export class ExchangeNode : public OpperationNode<Exchange const*>
{
public:
	explicit ExchangeNode(Exchange const* exchange) : _exchange(std::move(exchange)) {}
	AGIS_API ~ExchangeNode();

	Exchange const* evaluate() const noexcept override
	{
		return _exchange;
	}

	AGIS_API std::optional<UniquePtr<AssetLambdaReadNode>> create_asset_lambda_read_node(std::string const&, int) const noexcept;

private:
	Exchange const* _exchange;

};


//==================================================================================================
export class ExchangeViewNode : public ExpressionNode<std::optional<AgisException>>
{
public:
	AGIS_API ExchangeViewNode(
		SharedPtr<ExchangeNode const> exchange_node,
		UniquePtr<AssetLambdaNode> asset_lambda
	);

	AGIS_API std::optional<AgisException> evaluate() noexcept override;

	ExchangeView const& get_view() const { return _exchange_view; }

private:
	ExchangeView _exchange_view;
	Exchange const* _exchange;
	SharedPtr<ExchangeNode const> _exchange_node;
	UniquePtr<AssetLambdaNode> _asset_lambda;
	std::vector<Asset const*> _assets;
	size_t _warmup = 0;
	size_t _exchange_offset = 0;
 
};


}

}