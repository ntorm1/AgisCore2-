module;
#include <cmath>
#include <Eigen/Dense>
module AgisRiskModule:RiskAlloc;

namespace Agis
{

namespace Risk
{


//============================================================================
EigenVectorD
    risk_parity_weights_ccd_spinu(
        EigenVectorD const& weights,
        EigenMatrixD const& cov,
        const double tol,
        const int max_iter)
{
    assert(weights.size() == cov.rows());
    double aux, x_diff, xk_sum;
    auto n = weights.size();
    EigenVectorD xk = EigenVectorD::Constant(n, 1);
    xk = std::sqrt(1.0 / cov.sum()) * xk;
    EigenVectorD x_star(n), Sigma_xk(n), rc(n);
    Sigma_xk = cov * xk;
    for (auto k = 0; k < max_iter; ++k) {
        for (auto i = 0; i < n; ++i) {
            // compute update for the portfolio weights x
            aux = xk(i) * cov(i, i) - Sigma_xk(i);
            x_star(i) = (.5 / cov(i, i)) * (aux + std::sqrt(aux * aux + 4 * cov(i, i) * weights(i)));
            // update auxiliary terms
            x_diff = x_star(i) - xk(i);
            Sigma_xk += (cov.col(i).array() * x_diff).matrix();
            xk(i) = x_star(i);
        }
        xk_sum = xk.sum();
        rc = (xk.array() * (Sigma_xk).array() / (xk_sum * xk_sum)).matrix();
        if ((rc.array() / rc.sum() - weights.array()).abs().maxCoeff() < tol)
            break;
    }
    return x_star / xk_sum;
}


//============================================================================
EigenVectorD
    risk_parity_portfolio_ccd_choi(
        EigenVectorD const& weights,
        EigenMatrixD const& cov,
        const double tol,
        const int max_iter)
{
    assert(weights.size() == cov.rows());
    double ai;
    auto n = weights.size();
    auto vol = cov.diagonal().array().sqrt();
    auto invvol = (1 / vol.array()).matrix();
    EigenMatrixD corr = cov.array().colwise() * invvol.array();
    corr = corr.array().rowwise() * invvol.transpose().array();
    EigenMatrixD adj = corr;
    adj.diagonal().array() = 0;
    EigenVectorD wk = EigenVectorD::Ones(n);
    wk = (wk.array() / std::sqrt(corr.sum())).matrix();
    for (auto k = 0; k < max_iter; ++k) {
        for (auto i = 0; i < n; ++i) {
            // compute portfolio weights
            ai = 0.5 * ((adj.col(i).array() * wk.array()).sum());
            wk(i) = std::sqrt(ai * ai + weights(i)) - ai;
        }
        wk = wk.array() / std::sqrt(wk.transpose() * corr * wk);
        if ((wk.array() * (corr * wk).array() - weights.array()).abs().maxCoeff() < tol)
            break;
    }
    EigenVectorD w = wk.array() / vol.array();
    return (w / w.sum()).matrix();
}


//============================================================================
void
equal_volatility_weights(EigenVectorD& weights, EigenMatrixD const& cov)
{
    assert(weights.size() == cov.rows());
    auto n = weights.size();
    auto vol = cov.diagonal().array().sqrt();
    weights = (weights.array() / vol.array()).matrix();
    weights = (weights.array() / weights.sum()).matrix();
}


//============================================================================
void
vol_target_weights(EigenVectorD& weights, EigenMatrixD const& cov, const double vol_target)
{
    assert(weights.size() == cov.rows());
    auto current_vol = std::sqrt(weights.transpose() * cov * weights);
    double vol_scal = vol_target / current_vol;
    weights = (weights.array() * vol_scal).matrix();
}

}

}