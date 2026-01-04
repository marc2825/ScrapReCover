#pragma once
#include <array>
#include <vector>

#include <Siv3D.hpp>

#include "core/polygon.hpp"
#include "ui/ui_types.hpp"


// racks the outer "canvas" (placeable area) and its inner "layout" mask (target pattern region).
enum class CanvasMode { Waiting, InitialSolution, Optimization };

struct CanvasState {
    s3d::Grid<int> canvas_placement_count;
    std::vector<MyPolygon> placed_polygons;
    long long cur_loss = 0;
    int cur_waste = 0;
    int cur_empty_count = 0;
    std::vector<bool> is_locked;
    int locked_count = 0;
    CanvasMode mode = CanvasMode::Waiting;
    int iteration = 0;
    ui::PatternShapeType pattern_shape = ui::PatternShapeType::Square;
    s3d::Grid<bool> pattern_mask;
    int valid_cell_count = 0;
};

namespace canvas_state {

void Clear(CanvasState &state, int canvas_size, int max_polygons);

}
