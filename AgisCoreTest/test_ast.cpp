#include "pch.h"

import HydraModule;
import ExchangeMapModule;
import PortfolioModule;
import AgisStrategyTree;
import StrategyModule;
import SerializeModule;

using namespace Agis;

namespace AgisASTTest {

	std::string exchange1_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\AgisCoreTest\\data\\exchange1";
	std::string exchange_complex_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\AgisCoreTest\\data\\SPY_DAILY\\data.h5";

	std::string asset_id_1 = "test1";
	std::string asset_id_2 = "test2";
	std::string asset_id_3 = "test3";
	std::string portfolio_id_1 = "portfolio1";
	std::string exchange_id_1 = "exchange1";
	std::string exchange_id_complex = "SPY_DAILY";
	std::string strategy_id_1 = "strategy1";
	std::string dt_format = "%Y-%m-%d";
	double cash1 = 10000.0f;
	double cash2 = 20000.0f;

	constexpr long long t0 = 960181200000000000;
	constexpr long long t1 = 960267600000000000;
	constexpr long long t2 = 960354000000000000;
	constexpr long long t3 = 960440400000000000;
	constexpr long long t4 = 960526800000000000;
	constexpr long long t5 = 960786000000000000;
};

using namespace AgisASTTest;
using namespace Agis::AST;

class SimpleASTTests : public ::testing::Test
{
protected:
	std::shared_ptr<Hydra> hydra;
	Portfolio* portfolio1;
	size_t asset_index_1;
	size_t asset_index_2;
	size_t asset_index_3;
	std::unique_ptr<ExchangeViewNode> exchange_view_node;
	void SetUp() override
	{
		hydra = std::make_shared<Hydra>();
		auto res = hydra->create_exchange(
			exchange_id_1,
			dt_format,
			exchange1_path
		);
		auto res1 = hydra->create_portfolio(
			portfolio_id_1,
			exchange_id_1
		);
		portfolio1 = res1.value();
		auto& exchanges = hydra->get_exchanges();
		asset_index_1 = exchanges.get_asset_index(asset_id_1).value();
		asset_index_2 = exchanges.get_asset_index(asset_id_2).value();
		asset_index_3 = exchanges.get_asset_index(asset_id_3).value();

		auto exchange = hydra->get_exchange(exchange_id_1).value();
		auto exchange_node = std::make_shared<ExchangeNode>(exchange);
		auto previous_price = exchange_node->create_asset_lambda_read_node("CLOSE", -1);
		auto current_price = exchange_node->create_asset_lambda_read_node("CLOSE", 0);
		auto return_node = std::make_unique<AssetOpperationNode>(
			std::move(previous_price.value()),
			std::move(current_price.value()),
			AgisOperator::DIVIDE
		);
		exchange_view_node = std::make_unique<ExchangeViewNode>(exchange_node, std::move(return_node));
	}
};


class DummyASTStrategy : public Strategy {
public:
	DummyASTStrategy(
		std::string strategy_id,
		double cash,
		Exchange const& exchange,
		Portfolio& portfolio,
		std::unique_ptr<AllocationNode> node
	) : Strategy(strategy_id, cash, exchange, portfolio) 
	{
		this->_node = std::make_unique<StrategyNode>(*this, std::move(node), 0.0f);
	}

	std::expected<bool, AgisException> step() override {
		return _node->evaluate();
	}

private:
	std::unique_ptr<StrategyNode> _node;

};



TEST_F(SimpleASTTests, ExchangeView) {
	auto sort_node = std::make_unique<ExchangeViewSortNode>(
		std::move(exchange_view_node),
		ExchangeQueryType::NSmallest,
		1
	);
	auto alloc_node = std::make_unique<AllocationNode>(
		std::move(sort_node),
		AllocType::UNIFORM
	);
	auto strategy = std::make_unique<DummyASTStrategy>(
		strategy_id_1,
		cash1,
		*(hydra->get_exchange(exchange_id_1).value()),
		*portfolio1,
		std::move(alloc_node)
	);
	auto res_strat = hydra->register_strategy(std::move(strategy));
	hydra->build();
	hydra->step();
	EXPECT_DOUBLE_EQ(portfolio1->get_cash(), cash1);
	hydra->step();
	EXPECT_DOUBLE_EQ(portfolio1->get_cash(), 0.0f);

}