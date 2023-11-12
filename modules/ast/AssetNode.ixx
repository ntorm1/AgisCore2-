module;
#define NOMINMAX
#include "AgisDeclare.h"

export module AssetNode;

import <optional>;
import <string>;
import <variant>;

import BaseNode;

namespace Agis
{

namespace AST
{

enum class AssetLambdaType {
	READ,		///< Asset lambda read opp reads data from an asset at specific column and relative index
	OPP,		///< Asset lambda opp applies arithmatic opperation to two asset lambda nodes
	LOGICAL		///< Asset lambda logical opp compares asset lambda nodes to return a boolean value
};


//==================================================================================================
export class AssetLambdaNode : public OpperationNode<std::optional<double>,Asset const*>
{
public:
	AssetLambdaNode(AssetLambdaType asset_lambda_type_) noexcept :
		_asset_lambda_type(asset_lambda_type_)
	{};

	virtual ~AssetLambdaNode() {}
	size_t get_warmup() const noexcept { return this->_warmup; };

protected:
	void set_warmup(size_t w) { this->_warmup = w; };

private:
	AssetLambdaType _asset_lambda_type;
	size_t _warmup = 0;

};


//==================================================================================================
export class AssetLambdaReadNode : public AssetLambdaNode
{
public:
	AssetLambdaReadNode(size_t column, int index) noexcept:
		AssetLambdaNode(AssetLambdaType::READ),
		_column(column),
		_index(index)
	{
		this->set_warmup(abs(index));
	}

	std::optional<double> evaluate(Asset const* asset) const noexcept override;

protected:

private:
	size_t _column;
	int _index;
};


//==================================================================================================
export class AssetOpperationNode : public AssetLambdaNode
{
public:
	AssetOpperationNode(
		std::optional<UniquePtr<AssetLambdaNode>> left_node,
		UniquePtr<AssetLambdaReadNode>&& right_node,
		AgisOperation opp
	) noexcept : AssetLambdaNode(AssetLambdaType::OPP),
	_left_node(std::move(left_node)),
	_right_node(std::move(right_node)),
	_opp(opp)
	{
		auto w(std::max(_right_node->get_warmup(), _left_node ? _left_node.value()->get_warmup() : 0));
		this->set_warmup(w);
	}

	std::optional<double> evaluate(Asset const* asset) const noexcept override;


private:
	std::optional<UniquePtr<AssetLambdaNode>> _left_node;
	UniquePtr<AssetLambdaReadNode> _right_node;
	AgisOperation _opp;
};


//==================================================================================================
class AssetLambdaLogicalNode : public AssetLambdaNode
{
public:
	using AgisLogicalRightVal = std::variant<double, UniquePtr<AssetLambdaNode>>;
	AssetLambdaLogicalNode(
		UniquePtr<AssetLambdaNode> left_node,
		AgisLogicalOperation _opp,
		AgisLogicalRightVal right_node_,
		bool numeric_cast = false
	) noexcept;

	std::optional<double> evaluate(Asset const* asset) const noexcept override;

private:
	AgisLogicalOperation _opp;
	UniquePtr<AssetLambdaNode> _left_node;
	AgisLogicalRightVal _right_node;
	bool _numeric_cast = false;
};



}

}