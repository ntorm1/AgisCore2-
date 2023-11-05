#include "pch.h"

import HydraModule;
import ExchangeMapModule;
import ExchangeModule;
import PortfolioModule;
import StrategyModule;
import PositionModule;
import TradeModule;

using namespace Agis;

namespace AgisPortfolioTest {

	std::string exchange1_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\AgisCoreTest\\data\\exchange1";
	std::string exchange_complex_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\AgisCoreTest\\data\\SPY_DAILY\\data.h5";

	std::string asset_id_1 = "test1";
	std::string asset_id_2 = "test2";
	std::string asset_id_3 = "test3";
	std::string exchange_id_1 = "exchange1";
	std::string exchange_id_complex = "SPY_DAILY";
	std::string portfolio_id_1 = "portfolio1";
	std::string portfolio_id_2 = "portfolio2";
	std::string strategy_id_1 = "strategy1";
	std::string strategy_id_2 = "strategy2";
	std::string dt_format = "%Y-%m-%d";
	double cash1 = 1000.0f;
	double cash2 = 2000.0f;

	constexpr long long t0 = 960181200000000000;
	constexpr long long t1 = 960267600000000000;
	constexpr long long t2 = 960354000000000000;
	constexpr long long t3 = 960440400000000000;
	constexpr long long t4 = 960526800000000000;
	constexpr long long t5 = 960786000000000000;
};

using namespace AgisPortfolioTest;


class DummyStrategy : public Strategy {
public:
	DummyStrategy(
		std::string strategy_id,
		double cash,
		Exchange const& exchange,
		Portfolio& portfolio
	) : Strategy(strategy_id, cash, exchange, portfolio) {}

	std::expected<bool, AgisException> step() override {
		return true;
	}
	void place_market_order(std::string const& asset_id, double units) {
		auto index = this->get_asset_index(asset_id).value();
		Strategy::place_market_order(index, units);
	}
};


class PortfolioTest : public ::testing::Test
{
protected:
	std::shared_ptr<Hydra> hydra;
	Exchange const* exchange1;
	Portfolio const* master_portfolio;
	Portfolio* portfolio1;
	Portfolio* portfolio2;
	DummyStrategy* strategy1;
	DummyStrategy* strategy2;
	size_t strategy_index_1;
	size_t strategy_index_2;

	void SetUp() override
	{
		hydra = std::make_shared<Hydra>();
		auto res = hydra->create_exchange(
			exchange_id_1,
			dt_format,
			exchange1_path
		);
		auto res1 =hydra->create_portfolio(
			portfolio_id_1,
			exchange_id_1
		);
		auto res2 = hydra->create_portfolio(
			portfolio_id_2,
			exchange_id_1
		);
		exchange1 = hydra->get_exchange(exchange_id_1).value();
		portfolio1 = res1.value();
		portfolio2 = res2.value();
		master_portfolio = hydra->get_portfolio("master").value();

		auto strategy = std::make_unique<DummyStrategy>(
			strategy_id_1,
			cash1,
			*exchange1,
			*portfolio1
		);
		auto strategy_2 = std::make_unique<DummyStrategy>(
			strategy_id_2,
			cash2,
			*exchange1,
			*portfolio2
		);
		auto res_strat = hydra->register_strategy(std::move(strategy));
		auto res_strat_2 = hydra->register_strategy(std::move(strategy_2));

		strategy1 = dynamic_cast<DummyStrategy*>(hydra->get_strategy_mut(strategy_id_1).value());
		strategy2 = dynamic_cast<DummyStrategy*>(hydra->get_strategy_mut(strategy_id_1).value());
		strategy_index_1 = strategy1->get_strategy_index();
auto	strategy_index_2 = strategy2->get_strategy_index();
	}
};



TEST_F(PortfolioTest, PortfolioCreate) {
	EXPECT_TRUE(hydra->get_portfolio(portfolio_id_1).has_value());
	EXPECT_TRUE(hydra->get_portfolio(portfolio_id_2).has_value());
}

TEST_F(PortfolioTest, PortfolioStep) {
	hydra->build();
	hydra->step();

	auto p1 = hydra->get_portfolio(portfolio_id_1).value();
	EXPECT_DOUBLE_EQ(p1->get_cash(), cash1);
	EXPECT_DOUBLE_EQ(p1->get_nlv(), cash1);

	auto p_master = hydra->get_portfolio("master").value();
	EXPECT_DOUBLE_EQ(p_master->get_cash(), cash1 + cash2);
	EXPECT_DOUBLE_EQ(p_master->get_nlv(), cash1 + cash2);
}

TEST_F(PortfolioTest, OrderPlace) {
	hydra->build();
	hydra->step();
	EXPECT_DOUBLE_EQ(portfolio1->get_cash(), cash1);
	double units = 1.0f;
	strategy1->place_market_order(asset_id_2, units);

	double market_price = 101.5;
	double new_cash = cash1 - market_price * units;
	EXPECT_DOUBLE_EQ(portfolio1->get_cash(), new_cash);
	EXPECT_DOUBLE_EQ(strategy1->get_cash(), new_cash);

	auto master_position = master_portfolio->get_position(asset_id_2).value();
	auto portfolio1_position = portfolio1->get_position(asset_id_2).value();
	EXPECT_DOUBLE_EQ(master_position->get_units(), units);
	EXPECT_DOUBLE_EQ(portfolio1_position->get_units(), units);
	EXPECT_DOUBLE_EQ(master_position->get_avg_price(), market_price);
	EXPECT_DOUBLE_EQ(portfolio1_position->get_avg_price(), market_price);

	auto trade_master = master_position->get_trade(strategy_index_1).value();
	auto trade_portfolio1 = portfolio1_position->get_trade(strategy_index_1).value();
	EXPECT_EQ(trade_master, trade_portfolio1);
	EXPECT_EQ(trade_master->get_units(), units);
	EXPECT_EQ(trade_portfolio1->get_units(), units);
	EXPECT_EQ(trade_master->get_avg_price(), market_price);
	EXPECT_EQ(trade_portfolio1->get_avg_price(), market_price);
}


TEST_F(PortfolioTest, OrderEval) {
	hydra->build();
	hydra->step();
	double units = 1.0f;
	double market_price = 101.5;
	double next_price = 99.0f;
	strategy1->place_market_order(asset_id_2, units);

	hydra->step();

	auto master_position = master_portfolio->get_position(asset_id_2).value();
	auto portfolio1_position = portfolio1->get_position(asset_id_2).value();
	auto trade_master = master_position->get_trade(strategy_index_1).value();
	double trade_nlv = units * next_price;
	double unrealized_pnl = (next_price - market_price) * units;
	EXPECT_DOUBLE_EQ(trade_master->get_nlv(), trade_nlv);
	EXPECT_DOUBLE_EQ(master_position->get_nlv(), trade_nlv);
	EXPECT_DOUBLE_EQ(portfolio1_position->get_nlv(), trade_nlv);
	EXPECT_DOUBLE_EQ(trade_master->get_unrealized_pnl(), unrealized_pnl);
	EXPECT_DOUBLE_EQ(master_position->get_unrealized_pnl(), unrealized_pnl);
	EXPECT_DOUBLE_EQ(portfolio1_position->get_unrealized_pnl(), unrealized_pnl);

	double nlv_1 = cash1 + unrealized_pnl;
	double nlv_master = cash1 + cash2 + unrealized_pnl;
	EXPECT_DOUBLE_EQ(portfolio1->get_nlv(), nlv_1);
	EXPECT_DOUBLE_EQ(portfolio2->get_nlv(), cash2);
	EXPECT_DOUBLE_EQ(master_portfolio->get_nlv(), nlv_master);
}

TEST_F(PortfolioTest, OrderIncrease) {
	hydra->build();
	hydra->step();
	double units1 = 1.0f;
	double units2 = 2.0f;
	double market_price = 101.5;
	double next_price = 99.0f;
	strategy1->place_market_order(asset_id_2, units1);

	hydra->step();
	strategy1->place_market_order(asset_id_2, units2);

	auto master_position = master_portfolio->get_position(asset_id_2).value();
	double master_nlv = (units1 + units2) * next_price;
	EXPECT_DOUBLE_EQ(master_position->get_nlv(), master_nlv);
	EXPECT_DOUBLE_EQ(master_position->get_units(), units1 + units2);
}

