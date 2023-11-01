#include "pch.h"

import Exchange;

using namespace Agis;

std::string exchange_id = "test";
std::string dt_format = "%Y-%m-%d";
std::string exchange_source = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\AgisCore\\AgisCoreTest\\data\\exchange1";


TEST(ExchangeTestSuite, ExchangeCreate) {
	auto exchange_map = ExchangeMap();
	auto res = exchange_map.create_exchange(
		exchange_id,
		dt_format,
		exchange_source
	);
	EXPECT_TRUE(res.has_value());
	auto exchange = res.value();

	EXPECT_TRUE(exchange_map.asset_exists("test1"));
	EXPECT_FALSE(exchange_map.asset_exists("test0"));
}