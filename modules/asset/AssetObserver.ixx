module;

#pragma once
#ifdef AGISCORE_EXPORTS
#define AGIS_API __declspec(dllexport)
#else
#define AGIS_API __declspec(dllimport)
#endif

#include "AgisDeclare.h"

export module AssetObserverModule;

import <span>;
import <string>;
import <variant>;
import <vector>;
import <unordered_map>;

import AgisPointersModule;

namespace Agis
{

export enum ObserverType : uint16_t
{
	Read,
	Variance,
	Covariance,
	Sum,
	Correlation,
	TsMax,
	TsArgMinMax
};


extern AGIS_API std::unordered_map<std::string, ObserverType> observer_type_map;


//============================================================================
export class AssetObserver
{
public:
	AGIS_API AssetObserver(Asset const& asset, ObserverType t);
	virtual ~AssetObserver() = default;
	virtual void on_step() noexcept= 0;
	virtual void on_reset() noexcept = 0;
	virtual size_t warmup() const noexcept = 0;
	virtual size_t hash() const noexcept = 0;
	virtual double value() const noexcept = 0;

	ObserverType type() const noexcept { return _type; }
	Asset const& asset() const { return _asset; }
protected:
	double const* _data_ptr;
	Asset const& _asset;
	ObserverType _type;
};


//============================================================================
export class SumObserver : public AssetObserver
{
public:
	SumObserver(Asset const& asset, size_t lookback, UniquePtr<AssetObserver> input);
private:

	void on_step() noexcept override;
	void on_reset() noexcept override;

	UniquePtr<AssetObserver> _input;
	size_t _lookback;
	size_t _current_index = 0;
	size_t _count = 0;
	double _sum = 0.0;
	std::vector<double> _buffer;
};


//============================================================================
export class AssetReadObserver : public AssetObserver
{
public:
	AGIS_API AssetReadObserver(Asset const& asset, size_t column) : AssetObserver(asset, ObserverType::Read), _column(column) {}
	AGIS_API void on_step() noexcept override;
	void on_reset() noexcept override {}
	size_t warmup() const noexcept override { return 0; }
	double value() const noexcept override { return _value; }
	AGIS_API size_t hash() const noexcept override;

	size_t column() const noexcept { return _column; }

private:
	size_t _column;
	double _value = 0;
};



//============================================================================
export class TsMaxObserver : public AssetObserver
{
public:
	TsMaxObserver(Asset const& asset, size_t lookback, UniquePtr<AssetObserver> input);
	void on_step() noexcept override;
	void on_reset() noexcept override;
	size_t warmup() const noexcept override { return _lookback; }
	double value() const noexcept override { return _max; }

private:
	UniquePtr<AssetObserver> _input;
	size_t _lookback;
	size_t _current_index = 0;
	size_t _max_index = 0;
	size_t _count = 0;
	double _max = 0.0;
	std::vector<double> _buffer;
};


//============================================================================
export class TsArgMaxObserver : public AssetObserver
{
public:
	AGIS_API TsArgMaxObserver(
		Asset const& asset,
		size_t lookback,
		UniquePtr<AssetObserver> input
	);
	void on_step() noexcept override;
	void on_reset() noexcept override;
	size_t warmup() const noexcept override { return _lookback; }
	size_t hash() const noexcept override;
	double value() const noexcept override { return static_cast<double>(_arg_index); }

private:
	UniquePtr<AssetObserver> _input;
	size_t _lookback;
	size_t _current_index = 0;
	size_t _count = 0;
	size_t _arg_index = 0;
	double _arg_value = 0.0f;
	std::vector<double> _buffer;
};


//============================================================================
export class CorrelationObserver : public AssetObserver
{
public:
	CorrelationObserver(
		Asset const& parent,
		size_t lookback,
		UniquePtr<AssetObserver> x_input,
		UniquePtr<AssetObserver> y_input
	);
	void on_step() noexcept override;
	void on_reset() noexcept override;
	size_t warmup() const noexcept override { return _lookback; }
	
private:
	UniquePtr<AssetObserver> _x_input;
	UniquePtr<AssetObserver> _y_input;
	size_t _lookback;
	size_t _count = 0;
	size_t _current_index = 0;
	double _sum_x;
	double _sum_y;
	double _sum_xy;
	double _sum_x_squared;
	double _sum_y_squared;
	std::vector<double> _x_buffer;
	std::vector<double> _y_buffer;
};


//============================================================================
export class ReturnsVarianceObserver : public AssetObserver
{
public:
	ReturnsVarianceObserver(Asset const& asset, size_t lockback);
	void on_step() noexcept override;
	void on_reset() noexcept override;
	size_t warmup() const noexcept override { return _lookback; }
	size_t hash() const noexcept override;
	double value() const noexcept override { return _variance; }
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


//============================================================================
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
	size_t warmup() const noexcept override { return _lookback; }
	size_t hash() const noexcept override;
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