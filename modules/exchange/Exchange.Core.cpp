module;

#include "AgisMacros.h"
#include "AgisDeclare.h"

module ExchangeModule;

import <filesystem>;

import AssetModule;
import AgisArrayUtils;

namespace fs = std::filesystem;


namespace Agis
{

struct ExchangePrivate
{
	std::string exchange_id;
	size_t exchange_index;
	std::string dt_format;

	AssetFactory* asset_factory;
	std::vector<Asset*> assets;

	std::vector<long long> dt_index;
	size_t current_index = 0;

	ExchangePrivate(
		std::string exchange_id,
		size_t exchange_index,
		std::string dt_format);

	~ExchangePrivate();
};

//============================================================================
ExchangePrivate::ExchangePrivate(
	std::string exchange_id,
	size_t exchange_index,
	std::string dt_format)
{
	exchange_id = exchange_id;
	exchange_index = exchange_index;
	dt_format = dt_format;
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

	this->build();
	return true;
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


}