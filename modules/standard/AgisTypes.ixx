export module AgisTypes;

import <expected>;
import <optional>;

export namespace Agis
{
	template <typename T, typename E>
	using Result = std::expected<T, E>;

	template <typename T>
	using Optional = std::optional<T>;
}