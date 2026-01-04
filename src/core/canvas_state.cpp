#include "core/canvas_state.hpp"


namespace canvas_state {

void Clear(CanvasState &state, int canvas_size, int max_polygons) {
    state.canvas_placement_count = s3d::Grid<int>(canvas_size, canvas_size, 0);
    state.placed_polygons.clear();
    state.is_locked.assign(std::max(0, max_polygons), false);
    state.locked_count = 0;
    state.mode = CanvasMode::Waiting;
    state.iteration = 0;
    state.valid_cell_count = 0;
    state.cur_loss = 0;
    state.cur_waste = 0;
    state.cur_empty_count = 0;
}

}
