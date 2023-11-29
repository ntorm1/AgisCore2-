module;

#include "AgisDeclare.h"

export module AssetObserverModule;

import <span>;
import <string>;
import <variant>;
import <vector>;

import AgisPointersModule;

namespace Agis
{

export enum ObserverType : uint16_t
{
	Variance,
	Covariance,
	Sum,
};

export class AssetObserver
{
public:
	AssetObserver(Asset const& asset, ObserverType t);
	virtual ~AssetObserver() = default;
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



export class SumObserver : public AssetObserver
{
public:
	using ObserverInput = std::variant<UniquePtr<AssetObserver>, size_t>;
	SumObserver(Asset const& asset, size_t lookback, ObserverInput input);
private:

	void on_step() noexcept override;
	void on_reset() noexcept override;

	ObserverInput _input;
	size_t _lookback;
	size_t _current_index = 0;
	size_t _count = 0;
	double _sum = 0.0;
	std::vector<double> _buffer;
};


export class ReturnsVarianceObserver : public AssetObserver
{
public:
	ReturnsVarianceObserver(Asset const& asset, size_t lockback);
	void on_step() noexcept override;
	void on_reset() noexcept override;
	size_t warmup() noexcept override { return _lookback; }
	double value() const noexcept override { return _variance; }
	std::string str_rep() const noexcept override;

	void set_pointer(double* diagonal_ptr) { _diagnoal_ptr = diagonal_ptr; }

private:
	StridedPointer<double const> _span;
	size_t _count = 0;
	size_t _close_column_index;
	size_t _lookback;
	double _sum = 0.0;
	double _sum_of_squares = 0.0;
	double _variance = 0.0;
	double* _diagnoal_ptr = nullptr;
};


export class ReturnsCovarianceObserver : public AssetObserver
{ 

public:
	ReturnsCovarianceObserver(
		Asset const& parent,
		Asset const& child,
		size_t lookback,
		size_t step_size
	);

	/**
	 * @brief set the pointers into the covariance matrix that this incremental covariance struct will update
	 * @param upper_triangular_ pointer to the upper triangular portion of the covariance matrix
	 * @param lower_triangular_ pointer to the lower triangular portion of the covariance matrix
	*/
	void set_pointers(double* upper_triangular_, double* lower_triangular_);

private:
	void on_step() noexcept override;
	void on_reset() noexcept override;
	size_t warmup() noexcept override { return _lookback; }
	std::string str_rep() const noexcept;
	double value() const noexcept override { return _covariance; }

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

	/// <summary>
	/// Pointer to the location of this entry in the upper triangular portion of the covariance matrix
	/// </summary>
	double* _upper_triangular = nullptr;
	/// <summary>
	/// Symmetric pointer to the location of this entry in the lower triangular portion of the covariance matrix
	/// </summary>
	double* _lower_triangular = nullptr;
};


}