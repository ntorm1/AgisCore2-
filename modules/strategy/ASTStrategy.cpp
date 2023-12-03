module;
#include "AgisDeclare.h"
module ASTStrategyModule;

namespace Agis
{


//============================================================================
ASTStrategy::ASTStrategy(
	std::string strategy_id,
	double cash,
	Exchange const& exchange,
	Portfolio& portfolio) : Strategy(strategy_id, cash, exchange, portfolio)
{
}

//============================================================================
std::expected<bool, AgisException> ASTStrategy::step() noexcept
{
	return true;
}

}