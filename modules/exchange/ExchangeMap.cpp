module;

#include "AgisDeclare.h"
#include "AgisMacros.h"

module ExchangeMapModule;
import AssetModule;
import ExchangeModule;
import AgisArrayUtils;

namespace Agis
{


//============================================================================
std::expected<Exchange*, AgisException>
ExchangeFactory::create_exchange(
		std::string exchange_id,
		std::string dt_format,
		std::string source)
{
	auto exchange = Exchange::create(exchange_id, dt_format, _exchange_counter, source);
	auto res = exchange->load_assets();
	if (!res)
	{
		Exchange::destroy(exchange);
		return std::unexpected(res.error());
	}
	_exchange_counter++;
	return exchange;
}


//============================================================================
struct ExchangeMapPrivate
{
	ExchangeFactory factory;
	std::vector<long long> dt_index;
	std::unordered_map<std::string, size_t> asset_indecies;
	std::vector<Asset*> assets;
	std::vector<Exchange*> exchanges;
	std::unordered_map<std::string, size_t> exchange_indecies;

};


//============================================================================
std::expected<Exchange const*, AgisException>
ExchangeMap::create_exchange(std::string exchange_id, std::string dt_format, std::string source)
{
	// check if exchange already exists
	if (_p->exchange_indecies.find(exchange_id) != _p->exchange_indecies.end())
	{
		return std::unexpected(AgisException("Exchange already exists"));
	}

	// create the new exchange and copy over asset pointers
	AGIS_ASSIGN_OR_RETURN(exchange, _p->factory.create_exchange(exchange_id, dt_format, source));
	auto& exchange_assets = exchange->get_assets();
	for (auto& asset : exchange_assets)
	{
		_p->assets.push_back(asset);
		_p->asset_indecies.emplace(asset->get_id(), _p->assets.size() - 1);
	}
	// set the exchanges index offest. When exchange is indexing into it's asset vector
	// it will need to subtract this offset from the assets' index to get the correct index for it's vector
	exchange->set_index_offset(_p->assets.size() - _p->assets.size());
	
	// update the map with exchange info
	_p->exchanges.push_back(exchange);
	_p->exchange_indecies[exchange_id] = _p->exchanges.size() - 1;
	return exchange;
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
void
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
}


//============================================================================
bool
ExchangeMap::asset_exists(std::string const& id) const noexcept
{
	return this->_p->asset_indecies.find(id) != this->_p->asset_indecies.end();
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
std::expected<Exchange const*, AgisException>
ExchangeMap::get_exchange(std::string const& id) const noexcept
{
	auto it = _p->exchange_indecies.find(id);
	if (it == _p->exchange_indecies.end())
	{
		return std::unexpected(AgisException("Exchange does not exist"));
	}
	return _p->exchanges[it->second];
}




}