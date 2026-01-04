#include "algorithm/hyperparameter.hpp"

#include "utils/config.hpp"


// fallback
std::array<double, 5> Hyperparameter::P = {0.29, 0.30, 0.313, 0.081, 0.016};
std::array<double, 5> Hyperparameter::noP = {0.432, 0.447, 0, 0.121, 0};
long long Hyperparameter::np_pen = 13600;
long long Hyperparameter::or_pen = 250;
long long Hyperparameter::ol_pen = 100;
double Hyperparameter::sttmp = 17500.0;
double Hyperparameter::entmp = 5.0;

void Hyperparameter::LoadFromConfig() {
    P = Config::GetHyperparameterP();
    noP = Config::GetHyperparameterNoP();
    np_pen = Config::GetHyperparameterNpPen();
    or_pen = Config::GetHyperparameterOrPen();
    ol_pen = Config::GetHyperparameterOlPen();
    sttmp = Config::GetHyperparameterStTmp();
    entmp = Config::GetHyperparameterEnTmp();
}
