module;

#include "AgisDeclare.h"
#include "AgisMacros.h"

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

module SerializeModule;

import ExchangeModule;

namespace Agis
{

rapidjson::Document  serialize_exchange(rapidjson::Document::AllocatorType& a, Exchange const& exchange)
{
	auto exchange_id = exchange.get_exchange_id();
	auto source = exchange.get_source();
	auto dt_format = exchange.get_dt_format();
	rapidjson::Document j(rapidjson::kObjectType);
	j.AddMember("exchange_id", rapidjson::StringRef(exchange_id.c_str()), j.GetAllocator());
	j.AddMember("source_dir", rapidjson::StringRef(source.c_str()), j.GetAllocator());
	j.AddMember("dt_format", rapidjson::StringRef(dt_format.c_str()), j.GetAllocator());
	return j;
}

}