module;

#include <algorithm>

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
std::optional<double>
ExchangeView::get_allocation(size_t index) const noexcept
{
	if (index >= _view.size())
	{
		return std::nullopt;
	}
	auto& alloc = _view[index];
	if (alloc)
	{
		return alloc->amount;
	}
	return std::nullopt;
}


//============================================================================
std::optional<double>
ExchangeView::search_allocation(size_t asset_index) const noexcept
{
	auto it = std::find_if(_view.begin(), _view.end(), [&](auto& alloc) {
		return alloc && alloc->asset_index == asset_index;
		}
	);
	if (it != _view.end())
	{
		// test if nullopt
		if (*it) return (*it)->amount;
	}
	return std::nullopt;
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
	if (_view[index])
	{
		_view[index].value().amount = value;
	}
	else
	{
		_view[index] = Allocation(index, value);
		_allocation_count++;
	}
}


//============================================================================
bool compareBySecondValueAsc(const std::optional<Allocation>& a, const std::optional<Allocation>& b) {
	// If either a or b is empty, they should be considered equal
	if (!a && !b) return false;
	if (!a) return false;
	if (!b) return true;

	return a->amount < b->amount;
}

//============================================================================
bool compareBySecondValueDesc(const std::optional<Allocation>& a, const std::optional<Allocation>& b) {
	// If either a or b is empty, they should be considered equal
	if (!a && !b) return false;
	if (!a) return false;
	if (!b) return true;
	return a->amount > b->amount;
}


//============================================================================
void
ExchangeView::sort(size_t N, ExchangeQueryType sort_type)
{
	if (_view.size() <= N) { return; }
	switch (sort_type) {
	case(ExchangeQueryType::Default):
		_view.erase(_view.begin() + N, _view.end());
		_allocation_count = _view.size();
		return;
	case(ExchangeQueryType::NSmallest):
		std::partial_sort(
			_view.begin(),
			_view.begin() + N,
			_view.end(),
			compareBySecondValueAsc);
		_view.erase(_view.begin() + N, _view.end());
		_allocation_count = _view.size();
		return;
	case(ExchangeQueryType::NLargest):
		std::partial_sort(
			_view.begin(),
			_view.begin() + N,
			_view.end(),
			compareBySecondValueDesc);
		_view.erase(_view.begin() + N, _view.end());
		_allocation_count = _view.size();
		return;
	case(ExchangeQueryType::NExtreme): {
		auto n = N / 2;
		std::partial_sort(_view.begin(), _view.begin() + n, _view.end(), compareBySecondValueDesc);
		std::partial_sort(_view.begin() + n, _view.begin() + N, _view.end(), compareBySecondValueAsc);
		_view.erase(_view.begin() + n, _view.end() - n);
		_allocation_count = _view.size();
		return;
	}
	}
}


}