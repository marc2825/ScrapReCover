#pragma once
#include <Siv3D.hpp>

#include "algorithm/hyperparameter.hpp"
#include "core/canvas_model.hpp"
#include "utils/config.hpp"
#include "utils/layout_utils.hpp"


int CalcWaste();
int CalcWasteSingle(int x, int y);
int CountEmpty();
long long Eval();
long long EvalSingle(int x, int y);
void CanvasCountUpdate(int x, int y, int diff, long long &loss, int &empty_count, int &waste);
