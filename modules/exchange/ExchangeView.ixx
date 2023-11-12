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

export class ExchangeView
{
	friend class Exchange;
	friend class ExchangeViewNode;
public:
	using Allocation = std::optional<double>;
	ExchangeView(Exchange const* exchange);

	size_t size() const noexcept { return _view.size(); }
	size_t allocation_count() const noexcept { return _allocation_count; }
	AGIS_API std::optional<double> get_allocation(size_t index) const noexcept;

	void remove_allocation(size_t index);
	void set_allocation(size_t index, double value);

private:
	std::vector<Allocation> _view;
	size_t _allocation_count = 0;
	Exchange const* _exchange;

};


}