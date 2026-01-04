#include "algorithm/optimizer.hpp"

#include <cmath>
#include <random>

#include "core/canvas_model.hpp"

using namespace s3d;

// Linear Decay
static double Temperature(int iter, int maxiter) {
    return Hyperparameter::sttmp +
           (Hyperparameter::entmp - Hyperparameter::sttmp) *
               (static_cast<double>(iter) / maxiter);
}

// Exponential
bool Acceptance(int iter, long long cur_loss, long long nxt_loss,
                int maxiter) {
    if (cur_loss > nxt_loss) {
        return true;
    }

    double diff = nxt_loss - cur_loss;
    double temp = Temperature(iter, maxiter);
    double prob = std::exp(-diff / temp);
    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    return (dist01(CanvasModel::Get().GetRngOpt()) < prob);
}
