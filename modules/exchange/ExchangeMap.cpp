module;

#include "AgisDeclare.h"
#include "AgisMacros.h"

module ExchangeMapModule;
import AssetModule;
import OrderModule;
import ExchangeModule;
import AgisArrayUtils;

namespace Agis
{


//============================================================================
std::expected<UniquePtr<Exchange>, AgisException>
ExchangeFactory::create_exchange(
		std::string exchange_id,
		std::string dt_format,
		std::string source)
{
	auto exchange = Exchange::create(exchange_id, dt_format, _exchange_counter, source);
	auto res = exchange->load_assets();
	if (!res)
	{
		return std::unexpected(res.error());
	}
	_exchange_counter++;
	return std::move(exchange);
}


//============================================================================
struct ExchangeMapPrivate
{
	ExchangeFactory factory;
	
	std::vector<long long> dt_index;
	size_t current_index;
	long long global_dt = 0;

	std::unordered_map<std::string, size_t> asset_indecies;
	std::vector<Asset*> assets;
	std::vector<UniquePtr<Exchange>> exchanges;
	std::unordered_map<std::string, size_t> exchange_indecies;

};


//============================================================================
std::expected<Exchange const*, AgisException>
ExchangeMap::create_exchange(std::string exchange_id, std::string dt_format, std::string source)
{
	// check if exchange already exists
	if (_p->exchange_indecies.find(exchange_id) != _p->exchange_indecies.end())
	{
		return std::unexpected<AgisException>("Exchange already exists");
	}  

	// create the new exchange and copy over asset pointers
	AGIS_ASSIGN_OR_RETURN(exchange, _p->factory.create_exchange(exchange_id, dt_format, source));
	auto& exchange_assets = exchange->get_assets();
	for (auto& asset : exchange_assets)
	{
		_p->assets.push_back(asset.get());
		_p->asset_indecies.emplace(asset->get_id(), _p->assets.size() - 1);
	}
	// set the exchanges index offest. When exchange is indexing into it's asset vector
	// it will need to subtract this offset from the assets' index to get the correct index for it's vector
	exchange->set_index_offset(_p->assets.size() - _p->assets.size());
	
	// update the map with exchange info
	auto exchange_ptr = exchange.get();
	_p->exchanges.push_back(std::move(exchange));
	_p->exchange_indecies[exchange_id] = _p->exchanges.size() - 1;
	return exchange_ptr;
}


//============================================================================
std::vector<Asset*> const&
ExchangeMap::get_assets() const noexcept
{
	return _p->assets;
}


//============================================================================
ExchangeMap::ExchangeMap()
{
	_p = new ExchangeMapPrivate();
}


//============================================================================
ExchangeMap::~ExchangeMap()
{
	delete _p;
}


//============================================================================
std::expected<bool, AgisException>
ExchangeMap::build() noexcept
{
	this->_p->dt_index.clear();
	for (auto& exchange : _p->exchanges)
	{
		this->_p->dt_index = sorted_union(
			this->_p->dt_index,
			exchange->get_dt_index()
		);
	}
	return true;
}


//============================================================================
void
ExchangeMap::reset() noexcept
{
	_p->current_index = 0;
	_p->global_dt = 0;
	for (auto& exchange : _p->exchanges)
	{
		exchange->reset();
	}
}


//============================================================================
void ExchangeMap::process_orders(bool on_close) noexcept
{
	for (auto& exchange : _p->exchanges)
	{
		if (exchange->get_dt() == _p->global_dt)
		{
			exchange->process_orders(on_close);
		}
	}
}


//============================================================================
std::expected<bool, AgisException>
ExchangeMap::step() noexcept
{
	if (_p->current_index >= _p->dt_index.size())
	{
		return true;
	}
	_p->global_dt = _p->dt_index[_p->current_index];
	for (auto& exchange : _p->exchanges)
	{
		AGIS_ASSIGN_OR_RETURN(res, exchange->step(_p->global_dt));
	}
	_p->current_index++;
	return true;
}


//============================================================================
bool
ExchangeMap::asset_exists(std::string const& id) const noexcept
{
	return this->_p->asset_indecies.find(id) != this->_p->asset_indecies.end();
}


//============================================================================
std::unordered_map<std::string, size_t> const&
ExchangeMap::get_exchange_indecies() const noexcept
{
	return _p->exchange_indecies;
}


//============================================================================
std::expected<Asset const*, AgisException>
ExchangeMap::get_asset(std::string const& id) const noexcept
{
	if (!asset_exists(id))
	{
		return std::unexpected<AgisException>("Invalid Asset: " + id);
	}
	return this->_p->assets[this->_p->asset_indecies.at(id)];
}


//============================================================================
std::optional<size_t>
ExchangeMap::get_asset_index(std::string const& id) const noexcept
{
	if (_p->asset_indecies.count(id) == 0)
	{
		return std::nullopt;
	}
	return _p->asset_indecies.at(id);
}


//============================================================================
std::expected<Exchange const*, AgisException>
ExchangeMap::get_exchange(std::string const& id) const noexcept
{
	return this->get_exchange_mut(id);
}


//============================================================================
long long
ExchangeMap::get_global_time() const noexcept
{
	return _p->global_dt;
}


//============================================================================
long long
ExchangeMap::get_next_time() const noexcept
{
	if (_p->current_index >= _p->dt_index.size())
	{
		return 0;
	}
	return _p->dt_index[_p->current_index];
}


//============================================================================
std::vector<long long> const&
ExchangeMap::get_dt_index() const noexcept
{
	return _p->dt_index;
}


//============================================================================
std::expected<bool, AgisException>
ExchangeMap::force_place_order(Order* order, bool is_close) noexcept
{
	auto asset_index = order->get_asset_index();
	if (asset_index >= _p->assets.size())
	{
		return std::unexpected(AgisException("Invalid asset index"));
	}
	auto asset = _p->assets[asset_index];
	auto market_price = asset->get_market_price(is_close);
	if(!market_price)
	{
		return std::unexpected(AgisException("Invalid market price"));
	}
	order->fill(asset, market_price.value(), _p->global_dt);
	return true;
}


//============================================================================
std::expected<Exchange*, AgisException>
ExchangeMap::get_exchange_mut(std::string const& id) const noexcept
{
	auto it = _p->exchange_indecies.find(id);
	if (it == _p->exchange_indecies.end())
	{
		return std::unexpected(AgisException("Exchange does not exist"));
	}
	return _p->exchanges[it->second].get();
}



//============================================================================
std::optional<double>
ExchangeMap::get_market_price(std::string const& asset_id, bool close) const noexcept
{	
	auto it = _p->asset_indecies.find(asset_id);
	if (it == _p->asset_indecies.end())
	{
		return std::nullopt;
	}
	return _p->assets[it->second]->get_market_price(close);
}



}