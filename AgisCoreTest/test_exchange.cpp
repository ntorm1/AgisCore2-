#include "pch.h"

import HydraModule;
//import ExchangeMapModule;

using namespace Agis;

std::string exchange_id = "test";
std::string dt_format = "%Y-%m-%d";
std::string exchange_source = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\AgisCore\\AgisCoreTest\\data\\exchange1";


TEST(ExchangeTestSuite, ExchangeCreate) {
	auto hydra = Hydra();
	auto res = hydra.create_exchange(
		exchange_id,
		dt_format,
		exchange_source
	);
	EXPECT_TRUE(res.has_value());
	EXPECT_TRUE(hydra.asset_exists("test1"));
	EXPECT_FALSE(hydra.asset_exists("test0"));
}