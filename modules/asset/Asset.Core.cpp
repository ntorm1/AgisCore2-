module;

#include "AgisMacros.h"

#define READ_LOCK std::shared_lock<std::shared_mutex> lock(_mutex);
#define WRITE_LOCK std::unique_lock<std::shared_mutex> lock(_mutex);

module AssetModule;

import <optional>;

import AssetPrivateModule;
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
double*
Asset::get_data_ptr() const noexcept
{
	return _p->_data_ptr;
}


//============================================================================
StridedPointer<double const>
Asset::get_close_span() const noexcept
{
	return StridedPointer<double const>(
		_p->_data.data() + _p->_close_index,
		_p->_rows,
		_p->_cols
	);
}


//============================================================================
void
Asset::reset() noexcept
{
	_p->_current_index = 0;
	_p->_data_ptr = _p->_data.data();
	_state = AssetState::PENDING;
	if (!_p->observers.size()) return;
	for (auto& [id, observer] : _p->observers)
	{
		observer->on_reset();
	}
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
		advance();
		return _state;
	}
	auto asset_time = _p->_dt_index[_p->_current_index];
	switch (_state)
	{
		case AssetState::PENDING:
			if (asset_time == global_time)
			{
				_state = AssetState::STREAMING;
				advance();
			}
			break;
		case AssetState::STREAMING:
			if (asset_time != global_time)
			{
				_state = AssetState::DISABLED;
			}
			else
			{
				advance();
			}
			break;
		case AssetState::DISABLED:
			if (asset_time == global_time)
			{
				_state = AssetState::STREAMING;
				advance();
			}
			break;
		case AssetState::EXPIRED:
			break;
	}
	return _state;
}


//============================================================================
void
Asset::advance() noexcept
{
	_p->_data_ptr += _p->_cols;
	_p->_current_index++;
	if(!_p->observers.size()) return;
	for (auto& [id,observer] : _p->observers)
	{
		observer->on_step();
	}
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
bool
Asset::encloses(Asset const& other) const noexcept
{
	if(_p->_rows < other._p->_rows) return false;
	auto other_index = other.get_dt_index();
	auto other_start = get_enclosing_index(other);
	if (!other_start) return false;
	for (size_t i = 0; i < other.rows(); i++)
	{
		if (i > _p->_rows) return false;
		if (_p->_dt_index[*other_start + i] != other_index[i])
		{
			return false;
		}
	}
	return true;
}


//============================================================================
std::optional<size_t>
Asset::get_enclosing_index(Asset const& other) const noexcept
{
	auto other_index = other.get_dt_index();
	auto other_start = other_index.front();
	auto it = std::find(_p->_dt_index.begin(), _p->_dt_index.end(), other_start);
	if (it == _p->_dt_index.end()) return std::nullopt;
	return static_cast<size_t>(std::distance(_p->_dt_index.begin(), it));
}


//============================================================================
void
Asset::add_observer(UniquePtr<AssetObserver> observer) noexcept
{
	_p->add_observer(std::move(observer));
}



//============================================================================
std::optional<double>
Asset::get_pct_change(size_t column, size_t offset, size_t shift) const noexcept
{
#ifdef _DEBUG
	if (_p->_current_index == 0 || offset-shift > static_cast<int>(_p->_current_index - 1))
	{
		return std::nullopt;
	}
	if (offset == 0)
	{
		return 0.0;
	}
#endif
	auto current_idx = column - _p->_cols;
	auto prev_idx =  column - _p->_cols - offset * _p->_cols;
	current_idx -= shift * _p->_cols;
	prev_idx -= shift * _p->_cols;
	return (*(_p->_data_ptr + current_idx) - *(_p->_data_ptr + prev_idx)) / *(_p->_data_ptr + prev_idx);
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
std::optional<double> Asset::get_asset_feature(size_t column, int index) const noexcept
{
#ifdef _DEBUG
	if (_p->_current_index == 0 || abs(index) > static_cast<int>(_p->_current_index - 1) || index > 0)
	{
		return std::nullopt;
	}
	if (_state != AssetState::STREAMING && _state != AssetState::LAST)
	{
		return std::nullopt;
	}
	if (column >= _p->_cols)
	{
		return std::nullopt;
	}
#endif
	size_t index_offset = (index == 0) ? 0 : static_cast<size_t>(std::abs(index) * _p->_cols);
	return *(_p->_data_ptr + column - _p->_cols - index_offset);
}


//============================================================================
std::optional<size_t> Asset::get_streaming_index() const noexcept
{
	if(!this->is_streaming() || _p->_current_index == 0)
	{
		return std::nullopt;
	}
	return _p->_current_index - 1;
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
std::string const&
Asset::get_close_column() const noexcept
{
	return _p->_close_column;
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
size_t
Asset::get_current_index() const noexcept
{
	if (_p->_current_index == 0) return 0;
	return _p->_current_index - 1;
}


//============================================================================
size_t Asset::get_close_index() const noexcept
{
	return _p->_close_index;
}


//============================================================================
std::expected<UniquePtr<Asset>, AgisException>
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
	auto m = std::make_unique<Asset>(asset, asset_name, _asset_counter++);
	m->_dt_format = _dt_format;
	return m;
}


//============================================================================
std::expected<UniquePtr<Asset>, AgisException>
AssetFactory::create_asset(
	std::string asset_name,
	std::string source,
	H5::DataSet& dataset,
	H5::DataSpace& dataspace,
	H5::DataSet& datasetIndex,
	H5::DataSpace& dataspaceIndex)
{
	auto asset = new AssetPrivate();
	AGIS_ASSIGN_OR_RETURN(res, asset->load_h5(dataset, dataspace, datasetIndex, dataspaceIndex));
	auto m = std::make_unique<Asset>(asset, asset_name, _asset_counter++);
	m->_dt_format = _dt_format;
	return m;
}



};