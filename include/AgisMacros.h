#pragma once


#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b

#define AGIS_EXCEP(msg) \
    AgisException(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " - " + msg)

#define AGIS_ASSIGN_OR_RETURN(val, function) \
	auto CONCAT(val, _opt) = function; \
    if (!CONCAT(val, _opt)) { \
		return std::unexpected<AgisException>(CONCAT(val, _opt).error()); \
	} \
	auto val = std::move(CONCAT(val, _opt).value());


#define AGIS_OPTIONAL_REF(val, function) \
	auto CONCAT(val, _opt) = function; \
	if (!CONCAT(val, _opt)) { \
		return std::unexpected<AgisException>(AGIS_EXCEP("attempted nullopt unwrap")); \
	} \
	auto& val = CONCAT(val, _opt).value();

#define AGIS_OPTIONAL_MOVE(val, function) \
	auto CONCAT(val, _opt) = function; \
	if (!CONCAT(val, _opt)) { \
		return std::unexpected<AgisException>(AGIS_EXCEP("attempted nullopt unwrap")); \
	} \
	auto val = std::move(CONCAT(val, _opt).value());