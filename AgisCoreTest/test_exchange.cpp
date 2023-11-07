#include "pch.h"

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>

import HydraModule;
import ExchangeMapModule;
import SerializeModule;

using namespace Agis;

namespace AgisExchangeTest {

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

using namespace AgisExchangeTest;


class SimpleExchangeTests : public ::testing::Test
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



TEST_F(SimpleExchangeTests, ExchangeCreate) {
	EXPECT_TRUE(hydra->asset_exists("test1"));
	EXPECT_FALSE(hydra->asset_exists("test0"));
}


TEST_F(SimpleExchangeTests, ExchangeDtIndex) {
	hydra->build();
	auto& dt_index = hydra->get_dt_index();
	EXPECT_EQ(dt_index.size(), 6);
	EXPECT_EQ(dt_index[0], t0);
	EXPECT_EQ(dt_index[1], t1);
	EXPECT_EQ(dt_index[2], t2);
	EXPECT_EQ(dt_index[3], t3);
	EXPECT_EQ(dt_index[4], t4);
	EXPECT_EQ(dt_index[5], t5);
}


TEST_F(SimpleExchangeTests, TestGetMarketPrice) {
	hydra->build();
	auto& exchanges = hydra->get_exchanges();

	hydra->step();
	EXPECT_EQ(exchanges.get_global_time(), t0);
	EXPECT_FALSE(exchanges.get_market_price(asset_id_1, false).has_value());
	EXPECT_FALSE(exchanges.get_market_price(asset_id_1, true).has_value());
	EXPECT_EQ(exchanges.get_market_price(asset_id_2, false).value(), 101);
	EXPECT_EQ(exchanges.get_market_price(asset_id_2, true).value(), 101.5);

	hydra->step();

	EXPECT_EQ(exchanges.get_global_time(), t1);
	EXPECT_EQ(exchanges.get_market_price(asset_id_1, false).value(), 100);
	EXPECT_EQ(exchanges.get_market_price(asset_id_1, true).value(), 101);
	EXPECT_EQ(exchanges.get_market_price(asset_id_2, false).value(), 100);
	EXPECT_EQ(exchanges.get_market_price(asset_id_2, true).value(), 99);

	hydra->reset();
	hydra->step();
	EXPECT_EQ(exchanges.get_global_time(), t0);
	EXPECT_FALSE(exchanges.get_market_price(asset_id_1, false).has_value());
	EXPECT_FALSE(exchanges.get_market_price(asset_id_1, true).has_value());
	EXPECT_EQ(exchanges.get_market_price(asset_id_2, false).value(), 101);
	EXPECT_EQ(exchanges.get_market_price(asset_id_2, true).value(), 101.5);

	hydra->step();

	EXPECT_EQ(exchanges.get_global_time(), t1);
	EXPECT_EQ(exchanges.get_market_price(asset_id_1, false).value(), 100);
	EXPECT_EQ(exchanges.get_market_price(asset_id_1, true).value(), 101);
	EXPECT_EQ(exchanges.get_market_price(asset_id_2, false).value(), 100);
	EXPECT_EQ(exchanges.get_market_price(asset_id_2, true).value(), 99);
}

TEST_F(SimpleExchangeTests, TestRunTo) {
	hydra->build();
	auto& exchanges = hydra->get_exchanges();

	hydra->run_to(t4);
	EXPECT_EQ(exchanges.get_global_time(), t4);
	EXPECT_EQ(exchanges.get_market_price(asset_id_1, false).value(), 105.0f);
	EXPECT_EQ(exchanges.get_market_price(asset_id_1, true).value(), 106.0f);
	EXPECT_EQ(exchanges.get_market_price(asset_id_2, false).value(), 101.0f);
	EXPECT_EQ(exchanges.get_market_price(asset_id_2, true).value(), 101.5);

}

TEST_F(SimpleExchangeTests, TestExchangeMapSerialize) {
	// create rapid json doc and get the allocator
	rapidjson::Document doc;
	auto& allocator = doc.GetAllocator();
	auto res = serialize_hydra(allocator, *hydra, "test.json");
	EXPECT_TRUE(res.has_value());
	auto& json = res.value();
	EXPECT_TRUE(json.HasMember("exchanges"));

	// get the exchanges json
	auto& exchanges_json = json["exchanges"];
	EXPECT_TRUE(exchanges_json.IsObject());
	// print out all keys in the rapidjson object
	for (auto& key : exchanges_json.GetObject()) {
		auto s = key.name.GetString();
		int i = 1;
	}
	EXPECT_TRUE(exchanges_json.HasMember(exchange_id_1.c_str()));
}