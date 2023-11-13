module;


export module LogicalNode;

import <functional>;

namespace Agis
{

namespace AST 
{

export enum AgisLogicalOperator : uint8_t {
	GREATER_THAN,
	LESS_THAN,
	GREATER_THAN_OR_EQUAL,
	LESS_THAN_OR_EQUAL,
	EQUAL,
	NOT_EQUAL
};

export enum AgisOperator : uint8_t {
	INIT,
	IDENTITY,
	ADD,
	SUBTRACT,
	MULTIPLY,
	DIVIDE
};

}

}