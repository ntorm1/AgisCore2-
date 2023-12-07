module;
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif
#include "AgisDeclare.h"
#include "AgisAST.h"
export module StrategyNode;

import <expected>;

import BaseNode;
import AgisError;

namespace Agis
{


namespace AST
{

export class StrategyNode 
	: public ExpressionNode<std::expected<bool, AgisException>>
{
public:
	AGIS_API StrategyNode(Agis::ASTStrategy& strategy, UniquePtr<AllocationNode> alloc_node, double epsilon);
	AGIS_API virtual ~StrategyNode();

	AGIS_API std::expected<bool, AgisException> evaluate() noexcept override;
private:
	Agis::ASTStrategy& _strategy;
	UniquePtr<AllocationNode> _alloc_node;
	double _epsilon;
};



}

}