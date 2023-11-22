module;

#include "AgisDeclare.h"

export module AssetObserver;

namespace Agis
{

class AssetObserver
{
public:
	AssetObserver(Asset const& asset);
	virtual void on_step() noexcept= 0;
	virtual void on_reset() noexcept = 0;
	virtual size_t warmup() noexcept = 0;
	virtual double value() const noexcept = 0;

protected:
	Asset const& asset() const { return _asset; }
	double const* _data_ptr;
	Asset const& _asset;
};


class VarianceObserver : public AssetObserver
{
public:
	VarianceObserver(Asset const& asset, size_t lockback);
	void on_step() noexcept override;
	void on_reset() noexcept override;
	size_t warmup() noexcept override { return _lookback; }
	double value() const noexcept override { return _variance; }

private:
	size_t _count = 0;
	size_t _close_column_index;
	size_t _lookback;
	double _sum = 0.0;
	double _sum_of_squares = 0.0;
	double _variance = 0.0;
};


}