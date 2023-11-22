module AssetObserver;

import AssetModule;

namespace Agis
{

//============================================================================
AssetObserver::AssetObserver(Asset const& asset) : _asset(asset)
{
	_data_ptr = asset.get_data_ptr();
}


//============================================================================
VarianceObserver::VarianceObserver(Asset const& asset, size_t lockback) 
	: AssetObserver(asset), _lookback(lockback), _sum_of_squares(0.0)
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
	if (_count > _lookback)
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

}