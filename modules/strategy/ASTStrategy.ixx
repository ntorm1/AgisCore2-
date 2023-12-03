module;
#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif

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
		Portfolio& portfolio
	);

	std::expected<bool, AgisException> step() noexcept override;
};

}