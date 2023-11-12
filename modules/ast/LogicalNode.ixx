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

export typedef std::function<bool(double, double)> AgisLogicalOperation;


export inline const AgisLogicalOperation agis_greater_than = [](double a, double b) { return a > b; };
export inline const AgisLogicalOperation agis_less_than = [](double a, double b) { return a < b; };
export inline const AgisLogicalOperation agis_greater_than_or_equal = [](double a, double b) { return a >= b; };
export inline const AgisLogicalOperation agis_less_than_or_equal = [](double a, double b) { return a <= b; };
export inline const AgisLogicalOperation agis_equal = [](double a, double b) { return a == b; };
export inline const AgisLogicalOperation agis_not_equal = [](double a, double b) { return a != b; };
}

}