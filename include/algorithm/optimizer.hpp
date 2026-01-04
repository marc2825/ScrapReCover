#pragma once
#include <Siv3D.hpp>

#include "algorithm/hyperparameter.hpp"
#include "algorithm/optimizer_types.hpp"
#include "core/polygon.hpp"
#include "utils/config.hpp"


// Keeps optimization information per canvas set.
struct OptInfo {
    // Captures the best-known placement snapshot for one canvas.
    struct State {
        long long min_loss = 0;
        int min_waste = 0;
        int min_iter = 0;
        std::vector<MyPolygon> min_unplaced_polygons;
        std::vector<MyPolygon> min_placed_polygons;
        Grid<int> min_canvas_placement_count;
        bool has_value = false;
    };

    static void EnsureSize(int numCanvases);
    static void Resize(int numCanvases);
    static void Remove(int index);
    static State &Current();
    static State &At(int index);

  private:
    static std::vector<State> states_;
};

void SimulatedAnnealingStep();

void InitializeOptimization();
