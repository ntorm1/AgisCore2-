#include "pch.h"

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>

import HydraModule;
import ExchangeMapModule;
import SerializeModule;

using namespace Agis;

namespace AgisASTTest {

	std::string exchange1_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\AgisCoreTest\\data\\exchange1";
	std::string exchange_complex_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\AgisCoreTest\\data\\SPY_DAILY\\data.h5";

	std::string asset_id_1 = "test1";
	std::string asset_id_2 = "test2";
	std::string asset_id_3 = "test3";
	std::string exchange_id_1 = "exchange1";
	std::string exchange_id_complex = "SPY_DAILY";
	std::string dt_format = "%Y-%m-%d";

	constexpr long long t0 = 960181200000000000;
	constexpr long long t1 = 960267600000000000;
	constexpr long long t2 = 960354000000000000;
	constexpr long long t3 = 960440400000000000;
	constexpr long long t4 = 960526800000000000;
	constexpr long long t5 = 960786000000000000;
};

using namespace AgisASTTest;


class SimpleASTTests : public ::testing::Test
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
	}
};


TEST_F(SimpleASTTests, ExchangeView) {
	auto exchange = hydra->get_exchange(exchange_id_1);
	EXPECT_TRUE(exchange.has_value());
}