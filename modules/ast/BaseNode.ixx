module;
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif
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






} // namespace AST

} // namespace Agis
