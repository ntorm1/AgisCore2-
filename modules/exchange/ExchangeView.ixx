module;
#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif
#include "AgisDeclare.h"
#include "AgisAST.h"

export module ExchangeViewModule;

import <vector>;
import <optional>;

namespace Agis
{

export enum class ExchangeQueryType : uint8_t
{
	Default,	/// return all assets in view
	NLargest,	/// return the N largest
	NSmallest,	/// return the N smallest
	NExtreme	/// return the N/2 smallest and largest
};

enum class ExchangeViewOpp
{
	UNIFORM,			/// applies 1/N weight to each pair
};


export struct Allocation
{
	Allocation(size_t asset_index, double value) : asset_index(asset_index), amount(value) {}
	size_t asset_index;
	double amount;
};

// need to fx this

export class ExchangeView
{
	friend class Exchange;
	friend class AST::ExchangeViewNode;
	friend class AST::ExchangeViewSortNode;
public:
	
	ExchangeView(Exchange const* exchange);

	size_t size() const noexcept { return _view.size(); }
	size_t allocation_count() const noexcept { return _allocation_count; }
	std::vector<std::optional<Allocation>> const & get_view() const noexcept { return _view; }
	void set_weights(ExchangeViewOpp opp) noexcept;
	AGIS_API std::optional<double> get_allocation(size_t index) const noexcept;
	AGIS_API std::optional<double> search_allocation(size_t asset_index) const noexcept;

	void remove_allocation(size_t index);
	void set_allocation(size_t index, double value);

private:

	void uniform_weights();

	void sort(size_t count, ExchangeQueryType type);
	void set_allocation_count(size_t count) noexcept { _allocation_count = count; }

	std::vector<std::optional<Allocation>> _view;
	size_t _allocation_count = 0;
	Exchange const* _exchange;

};


}