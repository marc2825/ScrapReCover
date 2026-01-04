#include "algorithm/optimizer.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <random>

#include "algorithm/eval_functions.hpp"
#include "algorithm/optimizer_types.hpp"
#include "core/canvas_model.hpp"
#include "utils/layout_utils.hpp"
#include "utils/polygon_generator.hpp"


using namespace s3d;


namespace {

void NormalizeWeights(std::vector<float> &weights) {
    float sum = std::accumulate(weights.begin(), weights.end(), 0.0f);
    if (sum <= 0.0f && !weights.empty()) {
        const float uniform = 1.0f / static_cast<float>(weights.size());
        for (auto &w : weights) {
            w = uniform;
        }
    } else if (sum > 0.0f) {
        for (auto &w : weights) {
            w /= sum;
        }
    }
}

std::vector<float> BuildPlacedWeights(
    const std::vector<MyPolygon> &placed,
    const std::vector<bool> &locked_flags,
    const std::function<float(const MyPolygon &)> &weight_selector) {
    std::vector<float> weights;
    weights.reserve(placed.size());
    for (const auto &poly : placed) {
        const int idx = poly.GetIndex();
        const bool is_locked = (idx >= 0 && idx < static_cast<int>(locked_flags.size())) && locked_flags[idx];
        if (is_locked) {
            weights.emplace_back(0.0f);
        } else {
            weights.emplace_back(weight_selector(poly));
        }
    }
    NormalizeWeights(weights);
    return weights;
}

std::vector<float> BuildUnplacedWeights(const std::vector<MyPolygon> &unplaced) {
    std::vector<float> weights;
    weights.reserve(unplaced.size());
    for (const auto &poly : unplaced) {
        weights.emplace_back(poly.GetSelectionWeight());
    }
    NormalizeWeights(weights);
    return weights;
}

NeighborhoodOp Neighborhood() {
    NeighborhoodOp op;
    std::discrete_distribution<int> op_dist(Hyperparameter::P.begin(), Hyperparameter::P.end());
    std::discrete_distribution<int> op_dist_no(Hyperparameter::noP.begin(),
                                               Hyperparameter::noP.end());

    auto &model = CanvasModel::Get();
    auto &placed_polygons = model.GetPlacedPolygons();
    const auto &unplaced_polygons_ = model.GetUnplacedPolygonsConst();
    const auto &locked_flags = model.GetLockedFlagsConst();
    const int locked_count = model.GetLockedCount();

    int op_choice;
    if (unplaced_polygons_.empty())
        op_choice = op_dist_no(model.GetRngOpt());
    else
        op_choice = op_dist(model.GetRngOpt());
    op.op = static_cast<OperationType>(op_choice);

    if (static_cast<int>(placed_polygons.size()) == locked_count)
        op.op = OperationType::ADD;


    if (op.op == OperationType::MOVE) {
        const auto weights = BuildPlacedWeights(placed_polygons, locked_flags,
                                               [](const MyPolygon &) { return 1.0f; });
        std::discrete_distribution<> index_dist1(weights.begin(), weights.end());
        op.idx1 = index_dist1(model.GetRngOpt());

        std::uniform_int_distribution<int> dir_dist(0, 3);
        op.dir = dir_dist(model.GetRngOpt());

    } else if (op.op == OperationType::ROTATE) {
        const auto weights = BuildPlacedWeights(placed_polygons, locked_flags,
                                               [](const MyPolygon &) { return 1.0f; });
        std::discrete_distribution<> index_dist1(weights.begin(), weights.end());
        op.idx1 = index_dist1(model.GetRngOpt());

        std::uniform_real_distribution<double> angle_dist(0, 2 * Math::Pi);
        op.angle = angle_dist(model.GetRngOpt());

    } else if (op.op == OperationType::SWAP) {
        const auto placed_weights = BuildPlacedWeights(
            placed_polygons, locked_flags,
            [](const MyPolygon &p) { return 1.0f / static_cast<float>(p.GetSelectionWeight()); });
        std::discrete_distribution<> index_dist1(placed_weights.begin(), placed_weights.end());
        op.idx1 = index_dist1(model.GetRngOpt());

        const auto unplaced_weights = BuildUnplacedWeights(unplaced_polygons_);
        std::discrete_distribution<> index_dist2(unplaced_weights.begin(), unplaced_weights.end());
        op.idx2 = index_dist2(model.GetRngOpt());

        std::uniform_real_distribution<double> angle_dist(0, 2 * Math::Pi);
        op.angle = angle_dist(model.GetRngOpt());

    } else if (op.op == OperationType::ERASE) {
        const auto weights = BuildPlacedWeights(
            placed_polygons, locked_flags,
            [](const MyPolygon &p) { return 1.0f / static_cast<float>(p.GetSelectionWeight()); });
        std::discrete_distribution<> index_dist1(weights.begin(), weights.end());
        op.idx1 = index_dist1(model.GetRngOpt());

    } else if (op.op == OperationType::ADD) {
        const auto unplaced_weights = BuildUnplacedWeights(unplaced_polygons_);
        std::discrete_distribution<> index_dist2(unplaced_weights.begin(), unplaced_weights.end());
        op.idx2 = index_dist2(model.GetRngOpt());

        std::uniform_real_distribution<double> angle_dist(0, 2 * Math::Pi);
        op.angle = angle_dist(model.GetRngOpt());
        
        std::uniform_int_distribution<int> pos_dist(0, Config::GetLayoutSize() - 1);
        const auto &mask = model.GetPatternMaskConst();
        int offset = Config::GetCanvasMargin();
        int attempts = 0;
        int px = offset;
        int py = offset;
        bool found = false;
        while (!found) {
            int sx = pos_dist(model.GetRngOpt());
            int sy = pos_dist(model.GetRngOpt());
            px = sx + offset;
            py = sy + offset;
            if (px >= 0 && px < mask.width() && py >= 0 && py < mask.height() && mask[px][py]) {
                found = true;
            }
        }
        op.pos_x = px;
        op.pos_y = py;
    }
    return op;
}

} // namespace


bool Acceptance(int iter, long long cur_loss, long long nxt_loss,
                int maxiter = CanvasModel::Get().GetMaxIteration());

void SimulatedAnnealingStep() {
    NeighborhoodOp nb = Neighborhood();
    auto &model = CanvasModel::Get();
    auto &placed_polygons = model.GetPlacedPolygons();
    auto &unplaced_polygons_ = model.GetUnplacedPolygons();
    auto &cur_loss = model.GetCurLoss();
    auto &cur_empty_count = model.GetCurEmptyCount();
    int cur_waste = model.GetCurWaste();
    int &iteration = model.GetIterationRef();
    OptInfo::State &opt_state = OptInfo::Current();

    iteration++;

    long long nxt_loss = cur_loss;
    int nxt_empty_count = cur_empty_count;
    int nxt_waste = cur_waste;

    // Speedup using incremental updates
    if (nb.op == OperationType::MOVE) {
        for (const auto &p : placed_polygons[nb.idx1].GetRasterizedPoints())
            CanvasCountUpdate(p.x, p.y, -1, nxt_loss, nxt_empty_count, nxt_waste);
        placed_polygons[nb.idx1].Move(Vec2(dx[nb.dir], dy[nb.dir]));
        for (const auto &p : placed_polygons[nb.idx1].GetRasterizedPoints())
            CanvasCountUpdate(p.x, p.y, 1, nxt_loss, nxt_empty_count, nxt_waste);

    } else if (nb.op == OperationType::ROTATE) {
        for (const auto &p : placed_polygons[nb.idx1].GetRasterizedPoints())
            CanvasCountUpdate(p.x, p.y, -1, nxt_loss, nxt_empty_count, nxt_waste);
        placed_polygons[nb.idx1].Rotate(nb.angle);
        for (const auto &p : placed_polygons[nb.idx1].GetRasterizedPoints())
            CanvasCountUpdate(p.x, p.y, 1, nxt_loss, nxt_empty_count, nxt_waste);

    } else if (nb.op == OperationType::SWAP) {
        unplaced_polygons_[nb.idx2].SetRotation(nb.angle);
        unplaced_polygons_[nb.idx2].SetCenter(placed_polygons[nb.idx1].GetCenter());
        for (const auto &p : placed_polygons[nb.idx1].GetRasterizedPoints())
            CanvasCountUpdate(p.x, p.y, -1, nxt_loss, nxt_empty_count, nxt_waste);
        for (const auto &p : unplaced_polygons_[nb.idx2].GetRasterizedPoints())
            CanvasCountUpdate(p.x, p.y, 1, nxt_loss, nxt_empty_count, nxt_waste);
        std::swap(placed_polygons[nb.idx1], unplaced_polygons_[nb.idx2]);

    } else if (nb.op == OperationType::ERASE) {
        for (const auto &p : placed_polygons[nb.idx1].GetRasterizedPoints())
            CanvasCountUpdate(p.x, p.y, -1, nxt_loss, nxt_empty_count, nxt_waste);
        unplaced_polygons_.emplace_back(placed_polygons[nb.idx1]);
        placed_polygons.erase(placed_polygons.begin() + nb.idx1);

    } else if (nb.op == OperationType::ADD) {
        placed_polygons.emplace_back(unplaced_polygons_[nb.idx2]);
        placed_polygons.back().SetRotation(nb.angle);
        placed_polygons.back().SetCenter(Vec2(nb.pos_x, nb.pos_y));
        unplaced_polygons_.erase(unplaced_polygons_.begin() + nb.idx2);
        for (const auto &p : placed_polygons.back().GetRasterizedPoints())
            CanvasCountUpdate(p.x, p.y, 1, nxt_loss, nxt_empty_count, nxt_waste);
    }

    // Transitioning
    if (Acceptance(iteration, cur_loss, nxt_loss)) {
        cur_loss = nxt_loss;
        cur_waste = nxt_waste;
        cur_empty_count = nxt_empty_count;

        if (cur_empty_count == 0 &&
            (!opt_state.has_value || cur_waste < opt_state.min_waste)) {
            opt_state.min_loss = cur_loss;
            opt_state.min_waste = cur_waste;
            opt_state.min_iter = iteration;
            opt_state.min_unplaced_polygons = unplaced_polygons_;
            opt_state.min_placed_polygons = placed_polygons;
            opt_state.min_canvas_placement_count = model.GetCanvasPlacementCount();
            opt_state.has_value = true;
        }
        
    } else { // If not transitioning, roll back the state.
        if (nb.op == OperationType::MOVE) {
            for (const auto &p : placed_polygons[nb.idx1].GetRasterizedPoints())
                CanvasCountUpdate(p.x, p.y, -1, nxt_loss, nxt_empty_count, nxt_waste);
            placed_polygons[nb.idx1].Move(-Vec2(dx[nb.dir], dy[nb.dir]));
            for (const auto &p : placed_polygons[nb.idx1].GetRasterizedPoints())
                CanvasCountUpdate(p.x, p.y, 1, nxt_loss, nxt_empty_count, nxt_waste);

        } else if (nb.op == OperationType::ROTATE) {
            for (const auto &p : placed_polygons[nb.idx1].GetRasterizedPoints())
                CanvasCountUpdate(p.x, p.y, -1, nxt_loss, nxt_empty_count, nxt_waste);
            placed_polygons[nb.idx1].Rotate(-nb.angle);
            for (const auto &p : placed_polygons[nb.idx1].GetRasterizedPoints())
                CanvasCountUpdate(p.x, p.y, 1, nxt_loss, nxt_empty_count, nxt_waste);

        } else if (nb.op == OperationType::SWAP) {
            for (const auto &p : placed_polygons[nb.idx1].GetRasterizedPoints())
                CanvasCountUpdate(p.x, p.y, -1, nxt_loss, nxt_empty_count, nxt_waste);
            for (const auto &p : unplaced_polygons_[nb.idx2].GetRasterizedPoints())
                CanvasCountUpdate(p.x, p.y, 1, nxt_loss, nxt_empty_count, nxt_waste);
            std::swap(placed_polygons[nb.idx1], unplaced_polygons_[nb.idx2]);

        } else if (nb.op == OperationType::ERASE) {
            placed_polygons.emplace_back(unplaced_polygons_.back());
            unplaced_polygons_.pop_back();
            for (const auto &p : placed_polygons.back().GetRasterizedPoints())
                CanvasCountUpdate(p.x, p.y, 1, nxt_loss, nxt_empty_count, nxt_waste);
                
        } else if (nb.op == OperationType::ADD) {
            unplaced_polygons_.emplace_back(placed_polygons.back());
            for (const auto &p : unplaced_polygons_.back().GetRasterizedPoints())
                CanvasCountUpdate(p.x, p.y, -1, nxt_loss, nxt_empty_count, nxt_waste);
            placed_polygons.pop_back();
        }
    }

    model.SetCurWaste(cur_waste);
}
