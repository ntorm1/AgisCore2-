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
import <expected>;

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
	friend class ExchangeViewSortNode;
public:
	AGIS_API ExchangeViewNode(
		SharedPtr<ExchangeNode const> exchange_node,
		UniquePtr<AssetLambdaNode> asset_lambda
	);

	AGIS_API std::optional<AgisException> evaluate() noexcept override;

	size_t get_warmup() const { return _warmup; }
	ExchangeView const& get_view() const { return _exchange_view; }
	size_t size() const { return _exchange_view.size(); }

private:

	ExchangeView copy_view() { return _exchange_view; }

	ExchangeView _exchange_view;
	Exchange const* _exchange;
	SharedPtr<ExchangeNode const> _exchange_node;
	UniquePtr<AssetLambdaNode> _asset_lambda;
	std::vector<Asset const*> _assets;
	size_t _warmup = 0;
	size_t _exchange_offset = 0;
 
};


//==================================================================================================
export class ExchangeViewSortNode : public ExpressionNode<std::expected<ExchangeView, AgisException>>
{
public:
	ExchangeViewSortNode(
		UniquePtr<ExchangeViewNode> exchange_view_node,
		ExchangeQueryType query_type,
		int n
	) : _exchange_view_node(std::move(exchange_view_node)), _query_type(query_type)
	{
		_N = (n == -1) ? exchange_view_node->size() : static_cast<size_t>(n);
	}

	AGIS_API std::expected<ExchangeView, AgisException> evaluate() noexcept override;
	size_t get_warmup() const { return _exchange_view_node->get_warmup(); }

private:
	UniquePtr<ExchangeViewNode> _exchange_view_node;
	size_t _N;
	ExchangeQueryType _query_type;
};




}

}