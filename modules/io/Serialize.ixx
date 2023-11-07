module;
#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif

#include "AgisDeclare.h"

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

export module SerializeModule;

import <expected>;

import AgisError;

namespace Agis
{

typedef std::expected<rapidjson::Value, AgisException> SerializeResult;

rapidjson::Document serialize_exchange(Exchange const& exchange);

}
