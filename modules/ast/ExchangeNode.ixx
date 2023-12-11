module;
#define NOMINMAX
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif
#include <Eigen/Dense>
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

namespace Agis
{

namespace AST
{

//==================================================================================================
export class ExchangeNode : public OpperationNode<Exchange const*>
{
public:
	explicit ExchangeNode(Exchange const* exchange) 
		:	OpperationNode(NodeType::Exchange),
			_exchange(std::move(exchange)) {}
	AGIS_API virtual ~ExchangeNode() = default;

	Exchange const* evaluate() const noexcept override
	{
		return _exchange;
	}

	AGIS_API std::optional<UniquePtr<AssetLambdaReadNode>> create_asset_lambda_read_node(std::string const&, int) const noexcept;

private:
	Exchange const* _exchange;

};


//==================================================================================================
export class ExchangeViewNode : 
	public ExpressionNode<std::expected<Eigen::VectorXd const*, AgisException>>
{
	friend class ExchangeViewSortNode;
public:
	AGIS_API ExchangeViewNode(
		SharedPtr<ExchangeNode const> exchange_node,
		UniquePtr<AssetLambdaNode> asset_lambda
	);
	AGIS_API virtual ~ExchangeViewNode();
	AGIS_API std::expected<Eigen::VectorXd const*, AgisException> evaluate() noexcept override;

	size_t get_warmup() const { return _warmup; }
	size_t size() const { return _exchange_view.size(); }

private:
	Eigen::VectorXd _exchange_view;
	Exchange const* _exchange;
	SharedPtr<ExchangeNode const> _exchange_node;
	UniquePtr<AssetLambdaNode> _asset_lambda;
	std::vector<Asset const*> _assets;
	size_t _warmup = 0;
	size_t _exchange_offset = 0;
 
};


export enum class ExchangeQueryType : uint8_t
{
	Default,	/// return all assets in view
	NLargest,	/// return the N largest
	NSmallest,	/// return the N smallest
	NExtreme	/// return the N/2 smallest and largest
};


//==================================================================================================
export class ExchangeViewSortNode : 
	public ExpressionNode<std::expected<Eigen::VectorXd*,AgisException>>
{
public:
	AGIS_API ExchangeViewSortNode(
		UniquePtr<ExchangeViewNode> exchange_view_node,
		ExchangeQueryType query_type,
		int n
	) :	ExpressionNode(NodeType::ExchangeViewSort),
		_exchange_view_node(std::move(exchange_view_node)),
		_query_type(query_type)
	{
		_weights.resize(_exchange_view_node->size());
		_weights.setZero();
		_N = (n == -1) ? _exchange_view_node->size() : static_cast<size_t>(n);
	}
	AGIS_API virtual ~ExchangeViewSortNode();
	AGIS_API static std::unordered_map<std::string, ExchangeQueryType> const& ExchangeQueryTypeMap();
	AGIS_API std::expected<Eigen::VectorXd*, AgisException>  evaluate() noexcept override;
	size_t get_warmup() const { return _exchange_view_node->get_warmup(); }
	size_t view_size() const noexcept { return _view.size(); }


private:
	void sort(size_t count, ExchangeQueryType type);

	std::vector<std::pair<size_t, double>> _view;
	Eigen::VectorXd _weights;
	UniquePtr<ExchangeViewNode> _exchange_view_node;
	size_t _N;
	ExchangeQueryType _query_type;
};




}

}