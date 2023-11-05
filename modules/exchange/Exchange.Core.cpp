module;

#include "AgisMacros.h"
#include "AgisDeclare.h"

module ExchangeModule;

import <filesystem>;
import <unordered_map>;

import PortfolioModule;
import AssetModule;
import AgisArrayUtils;
import OrderModule;

namespace fs = std::filesystem;


namespace Agis
{

struct ExchangePrivate
{
	std::string exchange_id;
	size_t exchange_index;


	AssetFactory* asset_factory;
	std::vector<Asset*> assets;
	std::unordered_map<std::string, size_t> asset_index_map;
	std::vector<std::unique_ptr<Order>> orders;

	std::vector<long long> dt_index;
	long long current_dt = 0;
	size_t current_index = 0;
	std::string dt_format;
	bool on_close = false;

	ExchangePrivate(
		std::string exchange_id,
		size_t exchange_index,
		std::string dt_format);

	~ExchangePrivate();
};

//============================================================================
ExchangePrivate::ExchangePrivate(
	std::string _exchange_id,
	size_t _exchange_index,
	std::string _dt_format)
{
	exchange_id = _exchange_id;
	exchange_index = _exchange_index;
	dt_format = _dt_format;
	asset_factory = new AssetFactory(dt_format, exchange_id);
}

//============================================================================
ExchangePrivate::~ExchangePrivate()
{
	delete asset_factory;
	for (auto& asset : assets)
	{
		delete asset;
	}
}

//============================================================================
Exchange::Exchange(
	std::string exchange_id,
	size_t exchange_index,
	std::string dt_format,
	std::string source
)
{
	_source = source;
	_p = new ExchangePrivate(exchange_id, exchange_index, dt_format);
}


//============================================================================
std::expected<bool, AgisException> Exchange::load_folder() noexcept
{
	// get list of files in source directory
	auto source_path = fs::path(this->_source);
	auto files = fs::directory_iterator(source_path);
	
	// loop through files
	for (auto const& file : files)
	{
		// get file name minus the extension
		auto file_name = file.path().stem().string();
		AGIS_ASSIGN_OR_RETURN(asset, _p->asset_factory->create_asset(file_name, file.path().string()));
		_p->assets.push_back(asset);
	}

	return true;
}

//============================================================================
std::expected<bool, AgisException> Exchange::load_assets() noexcept
{
	// load source as fs path and check if it exists
	auto source_path = fs::path(this->_source);
	if (!fs::exists(source_path))
	{
		return std::unexpected(AgisException("Source path does not exist"));
	}

	// if source is a directory, call load_folder
	if (fs::is_directory(source_path))
	{
		AGIS_ASSIGN_OR_RETURN(res, this->load_folder());
	}
	else
	{
		return std::unexpected(AgisException("Source path is not a directory"));
	}

	for (auto& asset : _p->assets)
	{
		_p->asset_index_map.emplace(asset->get_id(), asset->get_index());
	}

	this->build();
	return true;
}


//============================================================================
void
Exchange::register_portfolio(Portfolio* p) noexcept
{
	registered_portfolios.emplace(p->get_portfolio_index(), p);
}


//============================================================================
std::expected<bool, AgisException>
Exchange::step(long long global_dt) noexcept
{
	if(this->_p->current_index >= this->_p->dt_index.size())
	{
		return true;
	}
	_p->current_dt = this->_p->dt_index[this->_p->current_index];
	if (_p->current_dt != global_dt)
	{
		return true;
	}
	// move assets forward
	for (auto& asset : this->_p->assets)
	{
		asset->step(global_dt);
	}
	// flag portfolios to call next step
	for (auto& portfolio : registered_portfolios)
	{
		portfolio.second->_step_call = true;
	}
	_p->current_index++;
	return true;
}


//============================================================================
void
Exchange::reset() noexcept
{
	for (auto& asset : this->_p->assets)
	{
		asset->reset();
	}
	this->_p->current_index = 0;
}

//============================================================================
void Exchange::build() noexcept
{
	this->_p->dt_index.clear();

	for (auto& asset : this->_p->assets) 
	{
		this->_p->dt_index = sorted_union(
			this->_p->dt_index,
			asset->get_dt_index()
		);
	}
}


//============================================================================
std::vector<Asset*>& Exchange::get_assets_mut() noexcept
{
	return _p->assets;
}


//============================================================================
size_t
Exchange::get_exchange_index() const noexcept
{
	return _p->exchange_index;
}

//============================================================================
long long Exchange::get_dt() const noexcept
{
	return _p->current_dt;
}


//============================================================================
std::optional<size_t>
Exchange::get_asset_index(std::string const& asset_id) const noexcept
{
	auto it = _p->asset_index_map.find(asset_id);
	if (it == _p->asset_index_map.end())
	{
		return std::nullopt;
	}
	return it->second;
}

//============================================================================
std::vector<long long> const&
Exchange::get_dt_index() const noexcept
{
	return _p->dt_index;
}

//============================================================================
std::vector<Asset*> const&
Exchange::get_assets() const noexcept
{
	return _p->assets;
}


//============================================================================
Exchange::~Exchange()
{
	delete this->_p;
}


//============================================================================
Exchange*
Exchange::create(
	std::string exchange_name,
	std::string dt_format,
	size_t exchange_index,
	std::string source)
{
	return new Exchange(exchange_name, exchange_index, dt_format, source);
}


//============================================================================
void Exchange::destroy(Exchange* exchange) noexcept
{
	if (exchange != nullptr)
	{
		delete exchange;
	}
}


//============================================================================
std::optional<Asset const*>
Exchange::get_asset(size_t asset_index) const noexcept
{
	// prevent underflow
	if (asset_index < this->_index_offset)
	{
		return std::nullopt;
	}

	// get the asset index offset to the exchange
	auto asset_index_offset = asset_index - this->_index_offset;
	if (asset_index_offset >= this->_p->assets.size())
	{
		return std::nullopt;
	}
	return this->_p->assets[asset_index_offset];
}


//============================================================================
std::optional<std::unique_ptr<Order>>
Exchange::place_order(std::unique_ptr<Order> order) noexcept
{
	// make sure order is valid
	if (!this->is_valid_order(order.get()))
	{
		order->reject(_p->current_dt);
		return std::move(order);
	}

	// attempt to fill order
	this->process_order(order.get());
	if (order->get_order_state() == OrderState::FILLED)
	{
		return std::move(order);
	}

	// otherwise store order in open orders
	order->set_order_state(OrderState::OPEN);
	this->_p->orders.push_back(std::move(order));
	return std::nullopt;
}


//============================================================================
void Exchange::process_market_order(Order* order) noexcept
{
	// get asset index
	auto asset_index = order->get_asset_index();
	asset_index -= this->_index_offset;

	// get asset
	auto asset = this->_p->assets[asset_index];

	// get asset price and fill if possible
	auto price = asset->get_market_price(_p->on_close);
	if (price)
	{
		order->fill(asset, price.value(), _p->current_dt);
	}
}


//============================================================================
void
Exchange::process_order(Order* order) noexcept
{
	switch (order->get_order_type())
	{
	case OrderType::MARKET_ORDER:
		this->process_market_order(order);
		break;
	}
}

//============================================================================
void
Exchange::process_orders(bool on_close) noexcept
{
	_p->on_close = on_close;
	for (auto orderIt = this->_p->orders.begin(); orderIt != this->_p->orders.end();)
	{
		auto& order = *orderIt;
		this->process_order(order.get());

		if (order->get_order_state() != OrderState::OPEN) {
			auto portfolio_index = order->get_portfolio_index();
			auto portfolio = registered_portfolios[portfolio_index];
			portfolio->process_order(std::move(order));

			// swap current order with last order and pop back
			std::iter_swap(orderIt, this->_p->orders.rbegin());
			this->_p->orders.pop_back();
		}
		else 
		{
			++orderIt;
		}
	}
}


//============================================================================
bool
Exchange::is_valid_order(Order const* order) const noexcept
{
	// validate order state, must be pending
	if (order->get_order_state() != OrderState::PENDING) 
	{
		return false;
	}

	// validate asset index 
	auto asset_index = order->get_asset_index();
	if (asset_index < this->_index_offset)
	{
		return false;
	}
	asset_index -= _index_offset;
	if (asset_index >= this->_p->assets.size())
	{
		return false;
	}

	// validate portfolio index
	auto portfolio_index = order->get_portfolio_index();
	if (registered_portfolios.find(portfolio_index) == registered_portfolios.end())
	{
		return false;
	}

	// validate asset is streaming
	auto asset = this->_p->assets[asset_index];
	if (asset->get_state() != AssetState::STREAMING)
	{
		return false;
	}
	return true;
}


}