module;

#include "AgisDeclare.h"

export module AssetObserverModule;

import <span>;
import <string>;

import AgisPointersModule;

namespace Agis
{

export enum ObserverType : uint16_t
{
	Variance,
	Covariance,
};

export class AssetObserver
{
public:
	AssetObserver(Asset const& asset, ObserverType t);
	virtual void on_step() noexcept= 0;
	virtual void on_reset() noexcept = 0;
	virtual size_t warmup() noexcept = 0;
	virtual std::string str_rep() const noexcept = 0;
	virtual double value() const noexcept = 0;

protected:
	Asset const& asset() const { return _asset; }
	double const* _data_ptr;
	Asset const& _asset;
	ObserverType _type;
};


export class VarianceObserver : public AssetObserver
{
public:
	VarianceObserver(Asset const& asset, size_t lockback);
	void on_step() noexcept override;
	void on_reset() noexcept override;
	size_t warmup() noexcept override { return _lookback; }
	double value() const noexcept override { return _variance; }
	std::string str_rep() const noexcept override;

private:
	size_t _count = 0;
	size_t _close_column_index;
	size_t _lookback;
	double _sum = 0.0;
	double _sum_of_squares = 0.0;
	double _variance = 0.0;
};


export class CovarianceObserver : public AssetObserver
{ 
private:
	CovarianceObserver(
		Asset const& parent,
		Asset const& child,
		size_t lookback,
		size_t step_size
	);

	void on_step() noexcept override;
	void on_reset() noexcept override;
	size_t warmup() noexcept override { return _lookback; }
	virtual std::string str_rep() const noexcept = 0;
	double value() const noexcept override { return _covariance; }

	/**
	 * @brief set the pointers into the covariance matrix that this incremental covariance struct will update
	 * @param upper_triangular_ pointer to the upper triangular portion of the covariance matrix
	 * @param lower_triangular_ pointer to the lower triangular portion of the covariance matrix
	*/
	void set_pointers(double* upper_triangular_, double* lower_triangular_);

	Asset const& _child;
	StridedPointer<double const> _enclosing_span;
	StridedPointer<double const> _child_span;
	size_t _enclosing_span_start_index;
	size_t _index = 0;
	size_t _lookback;
	size_t _step_size;
	double _sum1 = 0;
	double _sum2 = 0;
	double _sum_product = 0;
	double _sum1_squared = 0;
	double _sum2_squared = 0;
	double _covariance = 0;

	double* _upper_triangular = nullptr;
	double* _lower_triangular = nullptr;
};


}