#pragma once
#include <memory>
namespace Agis
{
	constexpr double UNIT_EPSILON = 1e-13;

	class Asset;

	template <typename T>
	using UniquePtr = std::unique_ptr<T>;

	template <typename T>
	using SharedPtr = std::shared_ptr<T>;

	class Exchange;
	class ExchangeMap;
	class ExchangeView;

	class Hydra;
	class Order;
	class Portfolio;
	class Position;
	class Trade;
	class Strategy;
	class StrategyPrivate;
}