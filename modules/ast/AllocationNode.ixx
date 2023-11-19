module;
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif
#include <Eigen/Dense>
#include "AgisDeclare.h"
#include "AgisAST.h"
export module AllocationNode;

import <expected>;

import BaseNode;
import AgisError;

namespace Agis
{


namespace AST
{

export enum class AllocType : uint8_t
{
	UNIFORM
};


export class AllocationNode: 
		public ExpressionNode<std::expected<Eigen::VectorXd*, AgisException>>
{
public:
	AGIS_API AllocationNode(
		UniquePtr<ExchangeViewSortNode> weights_node,
		AllocType alloc_type
	);
	
	AGIS_API virtual ~AllocationNode();

	std::expected<Eigen::VectorXd*, AgisException> evaluate() noexcept override;

private:
	void uniform_allocation(Eigen::VectorXd& weights);
	void set_weights(Eigen::VectorXd& weights);

	UniquePtr<ExchangeViewSortNode> _weights_node;
	double _epsilon;
	AllocType _alloc_type;
};




}

}