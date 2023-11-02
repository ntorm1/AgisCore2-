module;

#include "AgisMacros.h"

#define READ_LOCK std::shared_lock<std::shared_mutex> lock(_mutex);
#define WRITE_LOCK std::unique_lock<std::shared_mutex> lock(_mutex);

module AssetModule;

import <optional>;

import :AssetPrivateModule;

import AgisFileUtils;

namespace Agis
{


//============================================================================
Asset::Asset(AssetPrivate* asset, std::string asset_id, size_t asset_index)
{
	_asset = asset;
	_asset_id = asset_id;
	_asset_index = asset_index;
}


//============================================================================
Asset::~Asset()
{
	delete _asset;
}


//============================================================================
size_t Asset::get_index() const noexcept
{
	return size_t();
}


//============================================================================
size_t Asset::rows() const noexcept
{
	return _asset->_rows;
}


//============================================================================
size_t Asset::columns() const noexcept
{
	return _asset->_cols;
}


//============================================================================
std::vector<long long> const& Asset::get_dt_index() const noexcept
{
	return _asset->_dt_index;
}


//============================================================================
std::vector<double> const& Asset::get_data() const noexcept
{
	return _asset->_data;
}


//============================================================================
std::optional<std::vector<double>>
Asset::get_column(std::string const& column_name) const noexcept
{
	if (_asset->_headers.find(column_name) == _asset->_headers.end())
	{
		return std::nullopt;
	}
	auto col_index = _asset->_headers.at(column_name);
	std::vector<double> col;
	col.reserve(_asset->_rows);
	// loop over row major data in extract the column
	for (size_t row = 0; row < _asset->_rows; row++)
	{
		col.push_back(_asset->_data[_asset->get_index(row, col_index)]);
	}
	return col;
}


//============================================================================
std::vector<std::string>
Asset::get_column_names() const noexcept {
	std::vector<std::string> names;
	names.resize(_asset->_headers.size());  // Resize the vector to the required size

	for (const auto& [name, index] : _asset->_headers) {
		names[index] = name;
	}

	return names;
}

//============================================================================
std::expected<Asset*, AgisException>
AssetFactory::create_asset(
	std::string asset_name,
	std::string source
)
{
	auto asset = new AssetPrivate();
	AGIS_ASSIGN_OR_RETURN(file_type, Agis::get_file_type(source));
	switch (file_type)
	{
	case FileType::CSV:
		AGIS_ASSIGN_OR_RETURN(res, asset->load_csv(source, _dt_format));
		break;
	}
	auto m = new Asset(asset, asset_name, _asset_counter);
	m->_dt_format = _dt_format;
	_asset_counter++;
	return m;
}



};