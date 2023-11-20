module;
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif
#include "AgisAST.h"
export module BaseNode;

import <functional>;

namespace Agis
{

namespace AST
{

//============================================================================
class ASTNode {
public:
	ASTNode(NodeType type) : _type(type) {}
	virtual ~ASTNode() {}
private:
	NodeType _type;
};

//============================================================================
export template <typename T>
class ExpressionNode : public ASTNode {
public:
	ExpressionNode(NodeType type) : ASTNode(type) {}
	virtual ~ExpressionNode() {}
	virtual T evaluate() noexcept = 0;
};


//============================================================================
export template <typename Result, typename Param = void>
class OpperationNode : public ASTNode {
public:
	OpperationNode(NodeType type) : ASTNode(type) {}
	virtual ~OpperationNode() {}
	virtual Result evaluate(Param) const noexcept = 0;
};


//============================================================================
export class StatementNode : public ASTNode {
public:
	StatementNode(NodeType type) : ASTNode(type) {}
	virtual ~StatementNode() {}
	virtual void execute() = 0;
};






} // namespace AST

} // namespace Agis
