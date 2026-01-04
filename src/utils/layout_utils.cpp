#include "utils/layout_utils.hpp"

#include "core/canvas_model.hpp"

bool InLayout(int x, int y) {
    const auto &mask = CanvasModel::Get().GetPatternMaskConst();
    int width = static_cast<int>(mask.width());
    int height = static_cast<int>(mask.height());
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return false;
    }
    return mask[x][y];
}

bool IsEmpty(int x, int y) {
    const auto &canvas_placement_count = CanvasModel::Get().GetCanvasPlacementCountConst();
    return InLayout(x, y) && (canvas_placement_count[x][y] == 0);
}
