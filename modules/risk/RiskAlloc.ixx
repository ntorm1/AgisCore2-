export module AgisRiskModule:RiskAlloc;

import :RiskDeclare;

namespace Agis
{

namespace Risk
{

/// <summary>
/// Cyclical coordinate descent for Spinu's formulation of the risk parity portfolio problem
/// impl: https://github.com/mirca/riskparity.py/blob/master/riskparityportfolio/vanilla.h
/// Use equal risk contribution to scale the weights of a portfolio. 
/// </summary>
/// <param name="risk_budget"></param>
/// <param name="cov"></param>
/// <param name="tol"></param>
/// <param name="max_iter"></param>
/// <returns></returns>
EigenVectorD risk_parity_weights_ccd_spinu(
	EigenVectorD const& risk_budget,
	EigenMatrixD const& cov,
	const double tol,
	const int max_iter
);


/// <summary>
/// cyclical coordinate descent algo by Choi & Chen 2022
/// ref: https://arxiv.org/pdf/2203.00148.pdf
/// impl: https://github.com/mirca/riskparity.py/blob/master/riskparityportfolio/vanilla.h
/// Use equal risk contribution to determine the weights of a portfolio. The weights vector
/// passed serve as initial guess. The covariance matrix is passed as a parameter. The tolerance
/// </summary>
/// <param name="risk_budget"></param>
/// <param name="cov"></param>
/// <param name="tol"></param>
/// <param name="max_iter"></param>
/// <returns></returns>
EigenVectorD risk_parity_portfolio_ccd_choi(
	EigenVectorD const& risk_budget,
	EigenMatrixD const& cov,
	const double tol,
	const int max_iter
);


/// <summary>
/// Take a vector of weights and a covariance matrix and scale the weights by the 
/// sqaure root of the diagonal of the covariance matrix. Maintains the same sum and sign of weights.
/// </summary>
/// <param name="weights"></param>
/// <param name="cov"></param>
void vol_scale_weights(
	EigenVectorD& weights,
	EigenMatrixD const& cov
);


/// <summary>
/// Takes a vector of weights and a covariance matrix and scales the weights so that the the expected
/// variance of the portfolio is equal to the vol target. 
/// </summary>
/// <param name="weights"></param>
/// <param name="cov"></param>
/// <param name="vol_target"></param>
void vol_target_weights(
	EigenVectorD& weights,
	EigenMatrixD const& cov,
	const double vol_target
);

}

}