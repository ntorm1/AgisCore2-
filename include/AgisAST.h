#pragma once

#include <cstdint>

namespace Agis
{

namespace AST
{
	enum class NodeType
	{
		Unknown,
		Asset,
		AssetRead,
		AssetOpp,
		AssetLogical,
		Exchange,
		ExchangeView,
		ExchangeViewSort,
		Allocation,
		Strategy
	};

	enum AgisLogicalOperator : uint8_t {
		GREATER_THAN,
		LESS_THAN,
		GREATER_THAN_OR_EQUAL,
		LESS_THAN_OR_EQUAL,
		EQUAL,
		NOT_EQUAL
	};

	enum AgisOperator : uint8_t {
		INIT,
		IDENTITY,
		ADD,
		SUBTRACT,
		MULTIPLY,
		DIVIDE
	};

	class AssetLambdaNode;
	class AssetLambdaReadNode;

	class ExchangeNode;
	class ExchangeViewNode;
	class ExchangeViewSortNode;

	class AllocationNode;

	class StrategyNode;
}

}