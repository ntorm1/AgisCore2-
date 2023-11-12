module;

#include "AgisDeclare.h"

module ExchangeViewModule;

import ExchangeModule;

namespace Agis
{

//============================================================================
ExchangeView::ExchangeView(Exchange const* exchange)
	: _exchange(std::move(exchange))
{
	_view.resize(exchange->get_assets().size());
	std::fill(_view.begin(), _view.end(), std::nullopt);
}


//============================================================================
void ExchangeView::remove_allocation(size_t index)
{
	if (index >= _view.size())
	{
		return;
	}
	if (_view[index])
	{
		_view[index] = std::nullopt;
		_allocation_count--;
	}
}


//============================================================================
void ExchangeView::set_allocation(size_t index, double value)
{
	if (index >= _view.size())
	{
		return;
	}
	if (!_view[index])
	{
		_view[index] = value;
		_allocation_count++;
	}
	else
	{
		_view[index] = value;
	}
}

}