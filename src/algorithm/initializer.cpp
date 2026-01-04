#include "algorithm/initializer.hpp"

#include <numeric>
#include <random>

#include "algorithm/eval_functions.hpp"
#include "utils/polygon_generator.hpp"


bool LayoutInitializeStep() {
    int layout_size = Config::GetLayoutSize();
    int margin_size = Config::GetCanvasMargin();

    auto &model = CanvasModel::Get();
    auto &canvas_placement_count = model.GetCanvasPlacementCount();
    auto &placed_polygons = model.GetPlacedPolygons();
    auto &cur_loss = model.GetCurLoss();
    auto &cur_empty_count = model.GetCurEmptyCount();
    int cur_waste = model.GetCurWaste();
    auto &unplaced_polygons_ = model.GetUnplacedPolygons();

    Grid<double> pdf(layout_size, layout_size, 0.0);
    const auto &pattern_mask = model.GetPatternMaskConst();

    int pdf_sum = 0;
    int canvas_width = static_cast<int>(canvas_placement_count.width());
    int canvas_height = static_cast<int>(canvas_placement_count.height());

    for (int i = 0; i < layout_size; ++i) {
        for (int j = 0; j < layout_size; ++j) {
            int bx = i + margin_size;
            int by = j + margin_size;
            if (bx < 0 || bx >= canvas_width || by < 0 || by >= canvas_height) {
                continue;
            }
            if (!pattern_mask[bx][by]) {
                pdf[i][j] = 0.0;
                continue;
            }
            if (canvas_placement_count[bx][by] == 0) {
                pdf[i][j] = 1.0;
                pdf_sum++;
            }
        }
    }

    if (unplaced_polygons_.empty() || pdf_sum == 0)
        return true;

    for (int i = 0; i < layout_size; ++i) {
        for (int j = 0; j < layout_size; ++j) {
            pdf[i][j] /= pdf_sum;
        }
    }

    std::vector<float> unplaced_weights;
    unplaced_weights.reserve(unplaced_polygons_.size());
    for (const auto &p : unplaced_polygons_) {
        unplaced_weights.emplace_back(static_cast<float>(p.GetSelectionWeight()));
    }
    float sum_unplaced_weights = std::accumulate(unplaced_weights.begin(), unplaced_weights.end(), 0.0f);
    if (sum_unplaced_weights <= 0.0f) {
        return true;
    }
    for (auto &w : unplaced_weights) {
        w /= sum_unplaced_weights;
    }
    std::discrete_distribution<> unplaced_choice_dist(unplaced_weights.begin(), unplaced_weights.end());
    int index = unplaced_choice_dist(model.GetRngOpt());


    placed_polygons.emplace_back(unplaced_polygons_[index]);
    unplaced_polygons_.erase(unplaced_polygons_.begin() + index);

    std::vector<double> flat;
    flat.reserve(layout_size * layout_size);
    for (int i = 0; i < layout_size; ++i) {
        for (int j = 0; j < layout_size; ++j) {
            flat.push_back(pdf[i][j]);
        }
    }
    std::partial_sum(flat.begin(), flat.end(), flat.begin());
    std::uniform_real_distribution<double> rnd01(0.0, 1.0);
    double rnd = rnd01(model.GetRngOpt());
    auto it = std::lower_bound(flat.begin(), flat.end(), rnd);
    int pos = static_cast<int>(std::distance(flat.begin(), it));

    int cx = pos / layout_size;
    int cy = pos % layout_size;
    placed_polygons.back().SetCenter(Vec2(cx + margin_size, cy + margin_size));

    for (const auto &p : placed_polygons.back().GetRasterizedPoints()) {
        CanvasCountUpdate(p.x, p.y, 1, cur_loss, cur_empty_count, cur_waste);
    }
    model.SetCurWaste(cur_waste);

    return false;
}
