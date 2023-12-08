module;
#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif
#include <rapidjson/allocators.h>
#include <rapidjson/document.h>

#include "AgisDeclare.h"

export module ASTStrategyModule;

import AllocationNode;
import StrategyModule;

namespace Agis
{

export class ASTStrategy : public Strategy
{
public:
	AGIS_API ASTStrategy(
		std::string strategy_id,
		double cash,
		Exchange const& exchange,
		Portfolio& portfolio,
		std::string graph_file_path
	);
	AGIS_API ~ASTStrategy() = default;

	AGIS_API std::expected<bool, AgisException> step() noexcept override;
	
	void serialize(
		rapidjson::Document& document,
		rapidjson::Document::AllocatorType& allocator
	) const noexcept override;

	void set_alloc_node(UniquePtr<AST::AllocationNode> alloc_node) noexcept { _alloc_node = std::move(alloc_node); }
	void set_epsilon(double epsilon) noexcept { _epsilon = epsilon; }
	std::string const& graph_file_path() const noexcept { return _graph_file_path; }

private:
	std::string _graph_file_path;
	UniquePtr<AST::AllocationNode> _alloc_node = nullptr;
	double _epsilon = 0.0;

};

}