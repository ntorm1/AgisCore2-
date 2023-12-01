module;
#define NOMINMAX
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif
#include "AgisDeclare.h"
#include "AgisAST.h"
export module AssetNode;

import <optional>;
import <string>;
import <variant>;

import BaseNode;

namespace Agis
{

namespace AST
{


//==================================================================================================
export class AssetLambdaNode : public OpperationNode<std::optional<double>,Asset const*>
{
public:
	AssetLambdaNode(NodeType type) noexcept
		: OpperationNode(type)
	{};

	virtual ~AssetLambdaNode() {}
	size_t get_warmup() const noexcept { return this->_warmup; };

protected:
	void set_warmup(size_t w) { this->_warmup = w; };

private:
	size_t _warmup = 0;

};


//==================================================================================================
export class AssetLambdaReadNode : public AssetLambdaNode
{
public:
	AssetLambdaReadNode(size_t column, int index) noexcept:
		AssetLambdaNode(NodeType::AssetRead),
		_column(column),
		_index(index)
	{
		this->set_warmup(abs(index));
	}

	AGIS_API virtual ~AssetLambdaReadNode() = default;

	std::optional<double> evaluate(Asset const* asset) const noexcept override;

protected:

private:
	size_t _column;
	int _index;
};


//==================================================================================================
export class AssetObserverNode : public ExpressionNode<double>
{
public:
	AssetObserverNode(AssetObserver* observer);

	double evaluate() noexcept override;

private:
	AssetObserver* _observer;

};


//==================================================================================================
export class AssetOpperationNode : public AssetLambdaNode
{
public:
	AssetOpperationNode(
		std::optional<UniquePtr<AssetLambdaNode>> left_node,
		UniquePtr<AssetLambdaNode>&& right_node,
		AgisOperator opp
	) noexcept : AssetLambdaNode(NodeType::AssetOpp),
	_left_node(std::move(left_node)),
	_right_node(std::move(right_node)),
	_opp(opp)
	{
		auto w(std::max(_right_node->get_warmup(), _left_node ? _left_node.value()->get_warmup() : 0));
		this->set_warmup(w);
	}
	AGIS_API virtual ~AssetOpperationNode() = default;

	double execute_opperation(double left, double right) const noexcept;

	AGIS_API std::optional<double> evaluate(Asset const* asset) const noexcept override;


private:
	std::optional<UniquePtr<AssetLambdaNode>> _left_node;
	UniquePtr<AssetLambdaNode> _right_node;
	AgisOperator _opp;
};


//==================================================================================================
export class AssetLambdaLogicalNode : public AssetLambdaNode
{
public:
	using AgisLogicalRightVal = std::variant<double, UniquePtr<AssetLambdaNode>>;
	AssetLambdaLogicalNode(
		UniquePtr<AssetLambdaNode> left_node,
		AgisLogicalOperator _opp,
		AgisLogicalRightVal right_node_,
		bool numeric_cast = false
	) noexcept;
	AGIS_API virtual ~AssetLambdaLogicalNode() = default;

	bool execute_opperation(double left, double right) const noexcept;

	std::optional<double> evaluate(Asset const* asset) const noexcept override;

private:
	AgisLogicalOperator _opp;
	UniquePtr<AssetLambdaNode> _left_node;
	AgisLogicalRightVal _right_node;
	bool _numeric_cast = false;
};

}

}