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
	_p = asset;
	_p->_data_ptr = _p->_data.data();
	_asset_id = asset_id;
	_asset_index = asset_index;
}


//============================================================================
void
Asset::reset() noexcept
{
	_p->_current_index = 0;
	_p->_data_ptr = _p->_data.data();
	_state = AssetState::PENDING;
}


//============================================================================
AssetState
Asset::step(long long global_time) noexcept
{
	// if the asset is expired, return the state and do nothing
	if (_state == AssetState::LAST)
	{
		_state = AssetState::EXPIRED;
		return _state;
	}

	// check if last step to force close open positions
	if (_p->_current_index == _p->_dt_index.size() - 1)
	{
		_state = AssetState::LAST;
		_p->_current_index++;
		_p->_data_ptr += _p->_cols;
		return _state;
	}
	auto asset_time = _p->_dt_index[_p->_current_index];
	switch (_state)
	{
		case AssetState::PENDING:
			if (asset_time == global_time)
			{
				_state = AssetState::STREAMING;
				_p->_data_ptr += _p->_cols;
				_p->_current_index++;
			}
			break;
		case AssetState::STREAMING:
			if (asset_time != global_time)
			{
				_state = AssetState::DISABLED;
			}
			else
			{
				_p->_data_ptr += _p->_cols;
				_p->_current_index++;
			}
			break;
		case AssetState::DISABLED:
			if (asset_time == global_time)
			{
				_state = AssetState::STREAMING;
				_p->_data_ptr += _p->_cols;
				_p->_current_index++;
			}
			break;
		case AssetState::EXPIRED:
			break;
	}
	return _state;
}


//============================================================================
Asset::~Asset()
{
	delete _p;
}


//============================================================================
size_t Asset::rows() const noexcept
{
	return _p->_rows;
}


//============================================================================
size_t Asset::columns() const noexcept
{
	return _p->_cols;
}


//============================================================================
std::optional<double>
Asset::get_market_price(bool is_close) const noexcept
{
	if (_state != AssetState::STREAMING && _state != AssetState::LAST)
	{
		return std::nullopt;
	}
	if (is_close)
	{
		return *(_p->_data_ptr + _p->_close_index - _p->_cols);
	}
	else 
	{
		return *(_p->_data_ptr + _p->_open_index - _p->_cols);
	}
}


//============================================================================
std::vector<long long> const& Asset::get_dt_index() const noexcept
{
	return _p->_dt_index;
}


//============================================================================
std::vector<double> const& Asset::get_data() const noexcept
{
	return _p->_data;
}


//============================================================================
std::optional<std::vector<double>>
Asset::get_column(std::string const& column_name) const noexcept
{
	if (_p->_headers.find(column_name) == _p->_headers.end())
	{
		return std::nullopt;
	}
	auto col_index = _p->_headers.at(column_name);
	std::vector<double> col;
	col.reserve(_p->_rows);
	// loop over row major data in extract the column
	for (size_t row = 0; row < _p->_rows; row++)
	{
		col.push_back(_p->_data[_p->get_index(row, col_index)]);
	}
	return col;
}


//============================================================================
std::vector<std::string>
Asset::get_column_names() const noexcept {
	std::vector<std::string> names;
	names.resize(_p->_headers.size());  // Resize the vector to the required size

	for (const auto& [name, index] : _p->_headers) {
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