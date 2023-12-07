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
	std::string const& graph_file_path() const noexcept { return _graph_file_path; }

private:
	std::string _graph_file_path;

};

}