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
VarianceObserver::VarianceObserver(Asset const& asset, size_t lockback) 
	: AssetObserver(asset, ObserverType::Variance), _lookback(lockback), _sum_of_squares(0.0)
{
	_close_column_index = _asset.get_close_index();
}


//============================================================================
void VarianceObserver::on_step() noexcept
{
	_count++;
	if (_count == 1) return;
	double current_pct_change = *(_asset.get_pct_change(_close_column_index,1));
	_sum += current_pct_change;
	_sum_of_squares += current_pct_change * current_pct_change;
	if (_count > _lookback + 1)
	{
		double _mean = _sum / _lookback;
		_variance = (_sum_of_squares - _lookback * _mean * _mean) / (_lookback - 1);

		double old_pct_change = *(_asset.get_pct_change(_close_column_index,1, _lookback));
		_sum -= old_pct_change;
		_sum_of_squares -= old_pct_change * old_pct_change;
	}
}


//============================================================================
void VarianceObserver::on_reset() noexcept
{
	_count = 0;
	_variance = 0.0;
	_sum_of_squares = 0.0;
}


//============================================================================
std::string
VarianceObserver::str_rep() const noexcept
{
	return "VAR" + _asset.get_id() + std::to_string(_lookback);
}


//============================================================================
CovarianceObserver::CovarianceObserver(
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
}


//============================================================================
void
CovarianceObserver::on_step() noexcept
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
CovarianceObserver::on_reset() noexcept
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
CovarianceObserver::str_rep() const noexcept
{
	return  "COV_" + 
		_asset.get_id() + "_" +
		_child.get_id() + "_" +
		std::to_string(_lookback) + "_" +
		std::to_string(_step_size);
}


//============================================================================
void
CovarianceObserver::set_pointers(double* upper_triangular_, double* lower_triangular_)
{
	this->_upper_triangular = upper_triangular_;
	this->_lower_triangular = lower_triangular_;
}

}