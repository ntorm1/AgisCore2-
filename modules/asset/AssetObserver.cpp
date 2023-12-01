module AssetObserverModule;

import AssetModule;

namespace Agis
{

//============================================================================
AssetObserver::AssetObserver(Asset const& asset, ObserverType t) : _asset(asset), _type(t)
{
	_data_ptr = asset.get_data_ptr();
}


//============================================================================
SumObserver::SumObserver(Asset const& asset, size_t lockback, UniquePtr<AssetObserver> input)
	: AssetObserver(asset, ObserverType::Sum), _lookback(lockback), _input(std::move(input)), _sum(0.0)
{
	this->on_reset();
}


//============================================================================
void
SumObserver::on_step() noexcept
{
	_input->on_step();
	auto v = _input->value();
	if (_input->type() != ObserverType::Read)
	{
		// if add the end of the lookback period then remove the oldest value
		_buffer[_current_index] = v;
		auto next_index = (_current_index + 1) % _lookback;
		if (_count >= _lookback - 1)
		{
			_sum -= _buffer[_current_index];
		}
	}
	else if (_count >= _lookback - 1)
	{
		auto row = -1 * static_cast<int>(_lookback);
		auto p = static_cast<AssetReadObserver*>(_input.get());
		_sum -= _input->asset().get_asset_feature(p->column(), row).value();
	}
	_sum += v;
	_count++;
}


//============================================================================
void
SumObserver::on_reset() noexcept
{
	_input->on_reset();
	if (_input->type() != ObserverType::Read)
	{
		_buffer.clear();
		_buffer.resize(_lookback, 0.0f);
	}
	else if (_buffer.capacity())
	{
		_buffer.clear();
		_buffer.shrink_to_fit();
	}
	_sum = 0.0f;
	_count = 0;
	_current_index = 0;
}



//============================================================================
ReturnsVarianceObserver::ReturnsVarianceObserver(Asset const& asset, size_t lockback)
	:	AssetObserver(asset, ObserverType::Variance),
		_lookback(lockback),
		_sum_of_squares(0.0),
		_span(_asset.get_close_span())
{
	_close_column_index = _asset.get_close_index();
}


//============================================================================
void ReturnsVarianceObserver::on_step() noexcept
{
	if (_count == 0) {
		_count++;
		return;
	}
	double pct_change = (_span[_count] - _span[_count - 1]) / _span[_count - 1];
	_sum += pct_change;
	_sum_of_squares += pct_change * pct_change;
	if (_count < _lookback) {
		_count++;
		return;
	}
	// if out of warmup period then remove the previous value
	if (_count > _lookback) {
		size_t index_prev = (_count - 1) - (_lookback * 1 - 1);
		pct_change = (_span[index_prev] - _span[index_prev - 1]) / _span[index_prev - 1];
		_sum -= pct_change;
		_sum_of_squares -= pct_change * pct_change;
	}
	// calculate variance with updated sum and sum of squares
	double _mean = _sum / _lookback;
	_variance = (_sum_of_squares - _lookback * _mean * _mean) / (_lookback - 1);


	*_diagnoal_ptr = _variance;
	_count++;
}


//============================================================================
void ReturnsVarianceObserver::on_reset() noexcept
{
	_count = 0;
	_variance = 0.0;
	_sum_of_squares = 0.0;
	*_diagnoal_ptr = 0.0;
}


//============================================================================
std::string
ReturnsVarianceObserver::str_rep() const noexcept
{
	return "VAR" + _asset.get_id() + std::to_string(_lookback);
}

//============================================================================
ReturnsCovarianceObserver::ReturnsCovarianceObserver(
	Asset const& parent,
	Asset const& child,
	size_t lookback,
	size_t step_size
) : AssetObserver(parent, ObserverType::Covariance), 
	_child(child),
	_lookback(lookback),
	_step_size(step_size),
	_enclosing_span(_asset.get_close_span()),
	_child_span(_child.get_close_span())
{
	_enclosing_span_start_index = _asset.get_enclosing_index(_child).value();
}


//============================================================================
void
ReturnsCovarianceObserver::on_step() noexcept
{
	// If the current index is less than the enclosing span start index then return,
	// not in the time period in which the two assets have overlapping data
	if (this->_index <= this->_enclosing_span_start_index || this->_index < _step_size) {
		this->_index++;
		return;
	}

	// Check if on step
	if (this->_index % _step_size != 0) {
		this->_index++;
		return;
	}

	auto child_index = _index - this->_enclosing_span_start_index;
	double enclose_pct_change = (_enclosing_span[_index] - _enclosing_span[_index - _step_size]) / _enclosing_span[_index - _step_size];
	double child_pct_change = (_child_span[child_index] - _child_span[child_index - _step_size]) / _child_span[child_index - _step_size];
	this->_sum1 += enclose_pct_change;
	this->_sum2 += child_pct_change;
	this->_sum_product += enclose_pct_change * child_pct_change;
	this->_sum1_squared += enclose_pct_change * enclose_pct_change;
	this->_sum2_squared += child_pct_change * child_pct_change;

	// Check if in warmup
	if (child_index < this->_lookback * this->_step_size) {
		this->_index++;
		return;
	}

	// Check if need to remove the previous value
	if (child_index > this->_lookback) {
		size_t child_index_prev = (child_index - 1) - (this->_lookback * this->_step_size - _step_size);
		size_t index_prev = (_index - 1) - (this->_lookback * _step_size - _step_size);
		enclose_pct_change = (_enclosing_span[index_prev] - _enclosing_span[index_prev - _step_size]) / _enclosing_span[index_prev - _step_size];
		child_pct_change = (_child_span[child_index_prev] - _child_span[child_index_prev - _step_size]) / _child_span[child_index_prev - _step_size];
		this->_sum1 -= enclose_pct_change;
		this->_sum2 -= child_pct_change;
		this->_sum_product -= enclose_pct_change * child_pct_change;
		this->_sum1_squared -= enclose_pct_change * enclose_pct_change;
		this->_sum2_squared -= child_pct_change * child_pct_change;
	}

	// Set covariance and matrix pointers
	this->_covariance = (this->_sum_product - this->_sum1 * this->_sum2 / this->_lookback) / (this->_lookback - 1);
	*this->_upper_triangular = this->_covariance;
	*this->_lower_triangular = this->_covariance;
	this->_index++;
}
	

//============================================================================
void
ReturnsCovarianceObserver::on_reset() noexcept
{

	_sum1 = 0.0;
	_sum2 = 0.0;
	_sum1_squared = 0.0;
	_sum2_squared = 0.0;
	_covariance = 0.0;
	_sum_product = 0.0;
	_index = 0;
	if (this->_lower_triangular) {
		*this->_lower_triangular = 0.0f;
	}
	if (this->_upper_triangular) {
		*this->_upper_triangular = 0.0f;
	}
}


//============================================================================
std::string
ReturnsCovarianceObserver::str_rep() const noexcept
{
	return  "COV_" + 
		_asset.get_id() + "_" +
		_child.get_id() + "_" +
		std::to_string(_lookback) + "_" +
		std::to_string(_step_size);
}


//============================================================================
void
AssetReadObserver::on_step() noexcept
{
	_value = _asset.get_asset_feature(_column, 0).value();
}


//============================================================================
void
ReturnsCovarianceObserver::set_pointers(double* upper_triangular_, double* lower_triangular_)
{
	this->_upper_triangular = upper_triangular_;
	this->_lower_triangular = lower_triangular_;
}


//============================================================================
CorrelationObserver::CorrelationObserver(
	Asset const& parent,
	size_t lookback,
	UniquePtr<AssetObserver> x_input,
	UniquePtr<AssetObserver> y_input
)	: AssetObserver(parent, ObserverType::Correlation), _lookback(lookback), _x_input(std::move(x_input)), _y_input(std::move(y_input))
{
	if (_x_input->type() != ObserverType::Read)
	{
		_x_buffer.resize(_lookback, 0.0f);
	}
	if (_y_input->type() != ObserverType::Read)
	{
		_y_buffer.resize(_lookback, 0.0f);
	}
}


//============================================================================
void
CorrelationObserver::on_reset() noexcept
{
	_sum_x = _sum_y = _sum_xy = _sum_x_squared = _sum_y_squared = 0.0;
	_current_index = _count = 0;
	_x_input->on_reset();
	_y_input->on_reset();
	if (_x_input->type() != ObserverType::Read)
	{
		std::fill(_x_buffer.begin(), _x_buffer.end(), 0.0f);
	}
	else if (_x_buffer.capacity())
	{
		_x_buffer.clear();
		_x_buffer.shrink_to_fit();
	}
	if (_y_input->type() != ObserverType::Read)
	{
		std::fill(_y_buffer.begin(), _y_buffer.end(), 0.0f);
	}
	else if (_y_buffer.capacity())
	{
		_y_buffer.clear();
		_y_buffer.shrink_to_fit();
	}
}


//============================================================================
void
CorrelationObserver::on_step() noexcept
{
	_x_input->on_step();
	_y_input->on_step();
	auto x = _x_input->value();
	auto y = _y_input->value();
	_sum_x += x;
	_sum_y += y;
	_sum_xy += x * y;
	_sum_x_squared += x * x;
	_sum_y_squared += y * y;
}


}