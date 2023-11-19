#include "pch.h"

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <Eigen/Dense>

import HydraModule;
import ExchangeMapModule;
import AgisStrategyTree;
import SerializeModule;

using namespace Agis;
using namespace Agis::AST;

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
	constexpr double epsilon = 1e-7;  

};

using namespace AgisExchangeTest;


class SimpleExchangeTests : public ::testing::Test
{
protected:
	std::shared_ptr<Hydra> hydra;
	std::unique_ptr<ExchangeViewNode> exchange_view_node;
	size_t asset_index_1;
	size_t asset_index_2;
	size_t asset_index_3;
	void SetUp() override
	{
		hydra = std::make_shared<Hydra>();
		auto res = hydra->create_exchange(
			exchange_id_1,
			dt_format,
			exchange1_path
		);
		auto& exchanges = hydra->get_exchanges();
		asset_index_1 = exchanges.get_asset_index(asset_id_1).value();
		asset_index_2 = exchanges.get_asset_index(asset_id_2).value();
		asset_index_3 = exchanges.get_asset_index(asset_id_3).value();

		auto exchange = hydra->get_exchange(exchange_id_1).value();
		auto exchange_node = std::make_shared<ExchangeNode>(exchange);

		auto previous_price = exchange_node->create_asset_lambda_read_node("CLOSE", -1);
		auto current_price = exchange_node->create_asset_lambda_read_node("CLOSE", 0);
		EXPECT_TRUE(previous_price.has_value());
		EXPECT_TRUE(current_price.has_value());

		auto return_node = std::make_unique<AssetOpperationNode>(
			std::move(previous_price.value()),
			std::move(current_price.value()),
			AgisOperator::DIVIDE
		);
		exchange_view_node = std::make_unique<ExchangeViewNode>(exchange_node, std::move(return_node));
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

TEST_F(SimpleExchangeTests, TestExchangeMapSerialize) {	// create rapid json doc and get the allocator
	rapidjson::Document doc;
	auto& allocator = doc.GetAllocator();
	auto res = serialize_hydra(allocator, *hydra, "test.json");
	EXPECT_TRUE(res.has_value());
	auto& json = res.value();
	EXPECT_TRUE(json.HasMember("exchanges"));

	// get the exchanges json
	auto& exchanges_json = json["exchanges"];
	EXPECT_TRUE(exchanges_json.IsObject());
	EXPECT_TRUE(exchanges_json.HasMember(exchange_id_1.c_str()));

	auto new_hydra = deserialize_hydra("test.json");
	EXPECT_TRUE(new_hydra.has_value());
	auto& new_hydra_ref = new_hydra.value();
	auto& new_exchanges = new_hydra_ref->get_exchanges();
	EXPECT_TRUE(new_exchanges.asset_exists(asset_id_1));
	EXPECT_TRUE(new_exchanges.asset_exists(asset_id_2));
}

TEST_F(SimpleExchangeTests, TestExchangeViewNode) {
	hydra->build();
	auto res = exchange_view_node->evaluate();
	EXPECT_TRUE(res.has_value()); 
	auto& view = *(res.value());
	EXPECT_EQ(view.size(), 3);

	hydra->step();
	EXPECT_TRUE(exchange_view_node->evaluate().has_value());
	EXPECT_TRUE(std::isnan(view.sum()));

	hydra->step();
	EXPECT_TRUE(exchange_view_node->evaluate().has_value());
	EXPECT_TRUE(std::isnan(view[asset_index_1]));
	auto v2 = view[asset_index_2];
	auto v3 = view[asset_index_3];
	double v2_actual = 99.0f / 101.5f;
	double v3_actual = 101.4f / 101.5f;
	EXPECT_TRUE(abs(v2-v2_actual) < epsilon);
	EXPECT_TRUE(abs(v3-v3_actual) < epsilon);

	hydra->step();
	EXPECT_TRUE(exchange_view_node->evaluate().has_value());
	EXPECT_EQ((view.array() > 0).count() , 3);
	auto v1 = view[asset_index_1];
	v2 = view[asset_index_2];
	v3 = view[asset_index_3];
	auto v1_actual = 103.0f / 101.0f;
	v2_actual = 97.0f / 99.0f;
	v3_actual = 88.0f / 101.4f;
	EXPECT_TRUE(abs(v1 - v1_actual) < epsilon);
	EXPECT_TRUE(abs(v2 - v2_actual) < epsilon);
	EXPECT_TRUE(abs(v3 - v3_actual) < epsilon);
}


TEST_F(SimpleExchangeTests, TestExchangeViewNodeSort) 
{
	auto sort_node = std::make_unique<ExchangeViewSortNode>(
		std::move(exchange_view_node),
		ExchangeQueryType::NSmallest,
		1
	);
	hydra->build();
	hydra->step();
	hydra->step();
	auto view_opt = sort_node->evaluate();
	EXPECT_TRUE(view_opt.has_value());
	auto& weights = *(view_opt.value());
	EXPECT_EQ((weights.array() > 0).count(), 1);
	EXPECT_TRUE(std::isnan(weights[asset_index_1]));
	EXPECT_TRUE(std::isnan(weights[asset_index_3]));
	auto v2 = weights[asset_index_2];
	double v2_actual = 99.0f / 101.5f;
	EXPECT_TRUE(abs(v2 - v2_actual) < epsilon);

	hydra->step();
	auto now = std::chrono::system_clock::now();
	EXPECT_TRUE(sort_node->evaluate().has_value());
	auto then = std::chrono::system_clock::now();
	auto diff = std::chrono::duration_cast<std::chrono::microseconds>(then - now);
	EXPECT_EQ((weights.array() > 0).count(), 1);
	EXPECT_TRUE(std::isnan(weights[asset_index_1]));
	EXPECT_TRUE(std::isnan(weights[asset_index_2]));
	auto v3 = weights[asset_index_3];
	auto v3_actual = 88.0f / 101.4f;
	EXPECT_TRUE(abs(v2 - v2_actual) < epsilon);

}

