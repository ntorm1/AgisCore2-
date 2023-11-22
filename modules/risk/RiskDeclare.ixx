module;

#include <Eigen/Core>

export module AgisRiskModule:RiskDeclare;

export namespace Agis::Risk
{
	export using EigenVectorD = Eigen::Matrix<double, Eigen::Dynamic, 1>;
	export using EigenMatrixD = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>;
}