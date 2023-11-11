module;
#define NOMINMAX
#include "AgisDeclare.h"

export module AssetNode;

import <optional>;
import <string>;

import BaseNode;
import AgisPointer;

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
class AssetLambdaNode : public OpperationNode<std::optional<double>, Asset const*>
{
public:
	AssetLambdaNode(AssetLambdaType asset_lambda_type_) :
		_asset_lambda_type(asset_lambda_type_)
	{};

	virtual ~AssetLambdaNode() {}
	size_t get_warmup() const noexcept { return this->_warmup; };

protected:
	virtual void build() noexcept = 0;
	void set_warmup(size_t w) { this->_warmup = w; };

private:
	AssetLambdaType _asset_lambda_type;
	size_t _warmup = 0;

};


//==================================================================================================
export class AssetLambdaReadNode : public AssetLambdaNode
{
public:
	AssetLambdaReadNode(size_t column, int index) :
		AssetLambdaNode(AssetLambdaType::READ),
		_column(column),
		_index(index)
	{
		this->set_warmup(abs(index));
	}

	~AssetLambdaReadNode() {}

	void build() noexcept override {}
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
		NullableUniquePtr<AssetLambdaNode> left_node,
		NonNullUniquePtr<AssetLambdaReadNode> right_node,
		AgisOperation opp
	) : AssetLambdaNode(AssetLambdaType::OPP),
	_left_node(std::move(left_node)),
	_right_node(std::move(right_node)),
	_opp(opp)
	{
		auto w(std::max(_right_node->get_warmup(), _left_node ? _left_node.unwrap()->get_warmup() : 0));
		this->set_warmup(w);
	}

	~AssetOpperationNode() {}
	void build() noexcept override {}
	std::optional<double> evaluate(Asset const* asset) const noexcept override;


private:
	NullableUniquePtr<AssetLambdaNode> _left_node;
	NonNullUniquePtr<AssetLambdaReadNode> _right_node;
	AgisOperation _opp;
};


}

}