#include "algorithm/eval_functions.hpp"

#include <algorithm>

#include "algorithm/hyperparameter.hpp"


// Calculate actual waste
int CalcWaste() {
    int canvas_size = Config::GetCanvasSize();
    int waste = 0;
    for (int x = 0; x < canvas_size; ++x) {
        for (int y = 0; y < canvas_size; ++y) {
            waste += CalcWasteSingle(x, y);
        }
    }
    return waste;
}

int CalcWasteSingle(int x, int y) {
    const auto &canvas_placement_count = CanvasModel::Get().GetCanvasPlacementCountConst();
    if (InLayout(x, y)) {
        return std::max(canvas_placement_count[x][y] - 1, 0);
    } else {
        return canvas_placement_count[x][y];
    }
}

int CountEmpty() {
    int canvas_size = Config::GetCanvasSize();
    int cnt = 0;
    for (int x = 0; x < canvas_size; ++x) {
        for (int y = 0; y < canvas_size; ++y) {
            if (IsEmpty(x, y)) {
                cnt++;
            }
        }
    }
    return cnt;
}

// Calculate loss (i.e., surrogate objective)
long long Eval() {
    int canvas_size = Config::GetCanvasSize();

    long long loss = 0;
    for (int x = 0; x < canvas_size; ++x) {
        for (int y = 0; y < canvas_size; ++y) {
            loss += EvalSingle(x, y);
        }
    }
    return loss;
}

long long EvalSingle(int x, int y) {
    const auto &canvas_placement_count = CanvasModel::Get().GetCanvasPlacementCountConst();

    long long loss = 0;
    if (InLayout(x, y)) {
        if (canvas_placement_count[x][y] == 0) {
            loss += Hyperparameter::np_pen;
        } else {
            loss += (canvas_placement_count[x][y] - 1) * Hyperparameter::ol_pen;
        }
    } else {
        loss += canvas_placement_count[x][y] * Hyperparameter::or_pen;
    }
    return loss;
}

// Update differentially
void CanvasCountUpdate(int x, int y, int diff, long long &loss, int &empty_count, int &waste) {
    loss -= EvalSingle(x, y);
    empty_count -= IsEmpty(x, y);
    waste -= CalcWasteSingle(x, y);
    auto &canvas_placement_count = CanvasModel::Get().GetCanvasPlacementCount();
    canvas_placement_count[x][y] += diff;
    loss += EvalSingle(x, y);
    empty_count += IsEmpty(x, y);
    waste += CalcWasteSingle(x, y);
}
