#pragma once


#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b

#define AGIS_ASSIGN_OR_RETURN(val, function) \
	auto CONCAT(val, _opt) = function; \
    if (!CONCAT(val, _opt)) { \
		return std::unexpected<AgisException>(CONCAT(val, _opt).error()); \
	} \
	auto val = std::move(CONCAT(val, _opt).value());