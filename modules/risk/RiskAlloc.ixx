export module AgisRiskModule:RiskAlloc;

import :RiskDeclare;

namespace Agis
{

namespace Risk
{

//============================================================================
EigenVectorD risk_parity_weights_ccd_spinu(
	EigenVectorD const& weights,
	EigenMatrixD const& cov,
	const double tol,
	const int max_iter
);


//============================================================================
EigenVectorD risk_parity_portfolio_ccd_choi(
	EigenVectorD const& weights,
	EigenMatrixD const& cov,
	const double tol,
	const int max_iter
);


//============================================================================
void equal_volatility_weights(
	EigenVectorD& weights,
	EigenMatrixD const& cov
);


//============================================================================
void vol_target_weights(
	EigenVectorD& weights,
	EigenMatrixD const& cov,
	const double vol_target
);

}

}