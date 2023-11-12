module;

#pragma once
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


	void remove_allocation(size_t index);
	void set_allocation(size_t index, double value);

private:
	std::vector<Allocation> _view;
	size_t _allocation_count = 0;
	Exchange const* _exchange;

};


}