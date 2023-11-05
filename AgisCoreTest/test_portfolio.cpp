#include "pch.h"

import HydraModule;
import ExchangeMapModule;
import PortfolioModule;

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
	std::string dt_format = "%Y-%m-%d";

	constexpr long long t0 = 960181200000000000;
	constexpr long long t1 = 960267600000000000;
	constexpr long long t2 = 960354000000000000;
	constexpr long long t3 = 960440400000000000;
	constexpr long long t4 = 960526800000000000;
	constexpr long long t5 = 960786000000000000;
};

using namespace AgisPortfolioTest;


class PortfolioTest : public ::testing::Test
{
protected:
	std::shared_ptr<Hydra> hydra;

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
	}
};


TEST_F(PortfolioTest, ExchangeCreate) {
	EXPECT_TRUE(hydra->get_portfolio(portfolio_id_1).has_value());
	EXPECT_TRUE(hydra->get_portfolio(portfolio_id_2).has_value());
}
