export module BaseNode;

import <functional>;

namespace Agis
{

namespace AST
{

//============================================================================
class ASTNode {
public:
	virtual ~ASTNode() {}
};

//============================================================================
export template <typename T>
class ExpressionNode : public ASTNode {
public:
	virtual ~ExpressionNode() {}
	virtual T evaluate() noexcept = 0;
};


//============================================================================
export template <typename Result, typename Param = void>
class OpperationNode : public ASTNode {
public:
	virtual ~OpperationNode() {}
	virtual Result evaluate(Param) const noexcept = 0;
};


//============================================================================
export class StatementNode : public ASTNode {
public:
	virtual ~StatementNode() {}
	virtual void execute() = 0;
};


export typedef std::function<double(double, double)> AgisOperation;
export typedef std::function<bool(double, double)> AgisLogicalOperation;


export inline const AgisOperation agis_init = [](double a, double b) { return b; };
export inline const AgisOperation agis_identity = [](double a, double b) { return a; };
export inline const AgisOperation agis_add = [](double a, double b) { return a + b; };
export inline const AgisOperation agis_subtract = [](double a, double b) { return a - b; };
export inline const AgisOperation agis_multiply = [](double a, double b) { return a * b; };
export inline const AgisOperation agis_divide = [](double a, double b) { return a / b; };


export inline const AgisLogicalOperation agis_greater_than = [](double a, double b) { return a > b; };
export inline const AgisLogicalOperation agis_less_than = [](double a, double b) { return a < b; };
export inline const AgisLogicalOperation agis_greater_than_or_equal = [](double a, double b) { return a >= b; };
export inline const AgisLogicalOperation agis_less_than_or_equal = [](double a, double b) { return a <= b; };
export inline const AgisLogicalOperation agis_equal = [](double a, double b) { return a == b; };
export inline const AgisLogicalOperation agis_not_equal = [](double a, double b) { return a != b; };

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


} // namespace AST

} // namespace Agis
