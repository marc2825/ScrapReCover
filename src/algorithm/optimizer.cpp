#include "algorithm/optimizer.hpp"

#include <algorithm>

#include "core/canvas_model.hpp"
std::vector<OptInfo::State> OptInfo::states_;

void OptInfo::EnsureSize(int numCanvases) {
    int target = std::max(numCanvases, static_cast<int>(CanvasModel::Get().GetCanvasStateCount()));
    int clamped = std::max(1, target);
    if (static_cast<int>(states_.size()) < clamped) {
        states_.resize(clamped);
    }
}

OptInfo::State &OptInfo::Current() {
    EnsureSize(static_cast<int>(CanvasModel::Get().GetCanvasStateCount()));
    int index = CanvasModel::Get().GetCurrentCanvasIndex();
    index = std::clamp(index, 0, static_cast<int>(states_.size()) - 1);
    return states_[index];
}

OptInfo::State &OptInfo::At(int index) {
    EnsureSize(static_cast<int>(CanvasModel::Get().GetCanvasStateCount()));
    index = std::clamp(index, 0, static_cast<int>(states_.size()) - 1);
    return states_[index];
}

void OptInfo::Resize(int numCanvases) {
    int target = std::max(numCanvases, static_cast<int>(CanvasModel::Get().GetCanvasStateCount()));
    int clamped = std::max(1, target);
    states_.resize(clamped);
}

void OptInfo::Remove(int index) {
    if (index >= 0 && index < static_cast<int>(states_.size())) {
        states_.erase(states_.begin() + index);
    }
    if (states_.empty()) {
        states_.resize(1);
    }
}

void InitializeOptimization() {
    auto &placed_polygons = CanvasModel::Get().GetPlacedPolygons();
    OptInfo::State &state = OptInfo::Current();
    state.has_value = false;
    state.min_loss = 0;
    state.min_waste = 0;
    state.min_iter = 0;
    state.min_unplaced_polygons.clear();
    state.min_placed_polygons.clear();
    state.min_canvas_placement_count = Grid<int>();
    CanvasModel::Get().ResetIteration();
}
