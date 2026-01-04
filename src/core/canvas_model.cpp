#include "core/canvas_model.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "algorithm/initializer.hpp"
#include "algorithm/hyperparameter.hpp"
#include "algorithm/optimizer.hpp"
#include "core/canvas_state.hpp"
#include "utils/config.hpp"
#include "utils/pattern_mask.hpp"
#include "utils/polygon_generator.hpp"


namespace {
using namespace s3d;

FilePath ResolvePresetOutputsRoot() {
    Array<FilePath> candidates = Config::GetOutputsRoots();
    for (const auto& path : candidates) {
        if (path.isEmpty()) {
            continue;
        }
        if (FileSystem::IsDirectory(path)) {
            return FileSystem::FullPath(path);
        }
        FilePath absolute_root = FileSystem::FullPath(path);
        if (FileSystem::IsDirectory(absolute_root)) {
            return absolute_root;
        }
    }
    return U"";
}

FilePath ResolveCropperOutputsRoot() {
    const FilePath root = Config::GetMeasureOutputsDir();
    if (root.isEmpty()) {
        return U"";
    }
    if (FileSystem::IsDirectory(root)) {
        return FileSystem::FullPath(root);
    }
    FilePath absolute_root = FileSystem::FullPath(root);
    if (FileSystem::IsDirectory(absolute_root)) {
        return absolute_root;
    }
    return U"";
}

// Prevent duplicate loading.
bool IsFolderAlreadyLoaded(const FilePath& folder, const std::vector<MyPolygon>& unplaced_polygons) {
    for (const auto& poly : unplaced_polygons) {
        if (poly.GetTexturePath().has_value() &&
            FileSystem::ParentPath(poly.GetTexturePath().value()) == folder) {
            return true;
        }
    }
    return false;
}

// skip_loaded: true to ignore folders that have already been imported.
void TryAppendScrapFromFolder(const FilePath& folder,
                              bool skip_loaded,
                              std::vector<MyPolygon>& unplaced_polygons) {
    if (skip_loaded && IsFolderAlreadyLoaded(folder, unplaced_polygons)) {
        return;
    }

    FilePath json_path = FileSystem::PathAppend(folder, U"polygon_points.json");
    FilePath texture_path = FileSystem::PathAppend(folder, U"polygon_texture.png");

    if (!(FileSystem::Exists(json_path) && FileSystem::Exists(texture_path))) {
        std::cerr << "Missing polygon_points.json or polygon_texture.png in folder: "
                  << folder.narrow()
                  << '\n';
        return;
    }

    JSON json = JSON::Load(json_path);
    if (!json) {
        std::cerr << "Failed to load polygon_points.json: "
                  << json_path.narrow()
                  << '\n';
        return;
    }

    Array<Vec2> vertices;
    for (const auto& point : json.arrayView()) {
        double x = point[0].get<double>();
        double y = point[1].get<double>();
        vertices.emplace_back(Vec2(x, y));
    }

    Polygon cur_poly = Polygon(vertices);
    if (cur_poly.isEmpty()) { // Siv3D Polygons require specific winding order. If creation fails, reverse vertices and retry.
        std::reverse(vertices.begin(), vertices.end());
    }
    cur_poly = Polygon(vertices);

    if (cur_poly.area() == 0) {
        return;
    }

    unplaced_polygons.emplace_back(
        MyPolygon(cur_poly,
                  static_cast<int>(unplaced_polygons.size()),
                  Palette::Black,
                  texture_path));
}

} // namespace

CanvasModel &CanvasModel::Get() {
    static CanvasModel instance;
    return instance;
}

CanvasState &CanvasModel::CurrentCanvasState() {
    if (canvas_states_.empty()) {
        InitializeCanvasStates(std::max(1, Config::GetNumCanvases()));
    }
    current_canvas_index_ = std::clamp(
        current_canvas_index_, 0, static_cast<int>(canvas_states_.size()) - 1);
    return canvas_states_[current_canvas_index_];
}

const CanvasState &CanvasModel::CurrentCanvasStateConst() const {
    return const_cast<CanvasModel *>(this)->CurrentCanvasState();
}

void CanvasModel::InitializeCanvasStates(int numLayouts) {
    int clamped = std::max(1, numLayouts);
    canvas_states_.clear();
    canvas_states_.resize(clamped);
    OptInfo::Resize(clamped);
    for (int i = 0; i < clamped; ++i) {
        InitializeCanvasState(canvas_states_[i]);
        auto &opt_state = OptInfo::At(i);
        opt_state.min_loss = canvas_states_[i].cur_loss;
        opt_state.min_waste = canvas_states_[i].cur_waste;
        opt_state.min_iter = 0;
        opt_state.min_unplaced_polygons = unplaced_polygons_;
        opt_state.min_placed_polygons = canvas_states_[i].placed_polygons;
        opt_state.min_canvas_placement_count = canvas_states_[i].canvas_placement_count;
        opt_state.has_value = false;
    }
    current_canvas_index_ = 0;
}

void CanvasModel::InitializeCanvasState(CanvasState &state) {
    int canvas_size = Config::GetCanvasSize();
    canvas_state::Clear(state, canvas_size, Config::GetMaxPolygonCount());
    ApplyPatternShape(state, PatternShapeType::Square);
}

Grid<int> &CanvasModel::GetCanvasPlacementCount() {
    return CurrentCanvasState().canvas_placement_count;
}

const Grid<int> &CanvasModel::GetCanvasPlacementCountConst() const {
    return CurrentCanvasStateConst().canvas_placement_count;
}

std::vector<MyPolygon> &CanvasModel::GetPlacedPolygons() {
    return CurrentCanvasState().placed_polygons;
}

const std::vector<MyPolygon> &CanvasModel::GetPlacedPolygonsConst() const {
    return CurrentCanvasStateConst().placed_polygons;
}

std::vector<MyPolygon> &CanvasModel::GetUnplacedPolygons() {
    return unplaced_polygons_;
}

const std::vector<MyPolygon> &CanvasModel::GetUnplacedPolygonsConst() const {
    return unplaced_polygons_;
}

int CanvasModel::GetUnplacedPolygonNum() const {
    return static_cast<int>(unplaced_polygons_.size());
}

void CanvasModel::LoadAllScraps() {
    const s3d::FilePath root_folder = ResolvePresetOutputsRoot();
    if (root_folder.isEmpty()) {
        std::cerr << "[Load Scrap] outputs folder not found.\n";
        return;
    }

    for (const auto& folder : s3d::FileSystem::DirectoryContents(root_folder, s3d::Recursive::No)) {
        if (!s3d::FileSystem::IsDirectory(folder)) {
            continue;
        }
        TryAppendScrapFromFolder(folder, false, unplaced_polygons_);
    }
}

void CanvasModel::LoadNewScraps() {
    const s3d::FilePath root_folder = ResolveCropperOutputsRoot();
    if (root_folder.isEmpty()) {
        std::cerr << "[Load Scrap] outputs folder not found.\n";
        return;
    }

    for (const auto& folder : s3d::FileSystem::DirectoryContents(root_folder, s3d::Recursive::No)) {
        if (!s3d::FileSystem::IsDirectory(folder)) {
            continue;
        }
        TryAppendScrapFromFolder(folder, true, unplaced_polygons_);
    }
}

void CanvasModel::GeneratePolygons(int num) {
    for (int i = 0; i < num; ++i) {
        unplaced_polygons_.emplace_back(PolygonGenerator());
    }
}

bool CanvasModel::DeleteUnplacedScrapAt(int index) {
    if (index < 0 || index >= static_cast<int>(unplaced_polygons_.size())) {
        return false;
    }
    unplaced_polygons_.erase(unplaced_polygons_.begin() + index);
    return true;
}

void CanvasModel::CaptureCancelSnapshot() {
    cancel_snapshot_placed_ = GetPlacedPolygons();
    cancel_snapshot_unplaced_ = unplaced_polygons_;
    cancel_snapshot_grid_ = GetCanvasPlacementCount();
    cancel_snapshot_loss_ = GetCurLoss();
    cancel_snapshot_waste_ = GetCurWaste();
    cancel_snapshot_empty_count_ = GetCurEmptyCount();
    cancel_snapshot_valid_ = true;
}

void CanvasModel::RestoreCancelSnapshot() {
    if (!cancel_snapshot_valid_) {
        return;
    }
    GetPlacedPolygons() = cancel_snapshot_placed_;
    unplaced_polygons_ = cancel_snapshot_unplaced_;
    GetCanvasPlacementCount() = cancel_snapshot_grid_;
    GetCurLoss() = cancel_snapshot_loss_;
    SetCurWaste(cancel_snapshot_waste_);
    GetCurEmptyCount() = cancel_snapshot_empty_count_;
}

void CanvasModel::ClearCancelSnapshot() {
    cancel_snapshot_valid_ = false;
}

bool CanvasModel::HasCancelSnapshot() const {
    return cancel_snapshot_valid_;
}

bool CanvasModel::RunInitializationStep() {
    return LayoutInitializeStep();
}

bool CanvasModel::RunOptimizationStep() {
    SimulatedAnnealingStep();
    return GetIteration() == GetMaxIteration();
}

long long &CanvasModel::GetCurLoss() {
    return CurrentCanvasState().cur_loss;
}

int CanvasModel::GetCurWaste() const {
    return CurrentCanvasStateConst().cur_waste;
}

void CanvasModel::SetCurWaste(int waste) {
    CurrentCanvasState().cur_waste = waste;
}

int &CanvasModel::GetCurEmptyCount() {
    return CurrentCanvasState().cur_empty_count;
}

const std::vector<bool> &CanvasModel::GetLockedFlagsConst() const {
    return CurrentCanvasStateConst().is_locked;
}

bool CanvasModel::IsLocked(int index) const {
    const auto &flags = CurrentCanvasStateConst().is_locked;
    if (index < 0 || index >= static_cast<int>(flags.size())) {
        return false;
    }
    return flags[index];
}

void CanvasModel::SetLocked(int index, bool locked) {
    auto &state = CurrentCanvasState();
    auto &flags = state.is_locked;
    if (index < 0 || index >= static_cast<int>(flags.size())) {
        return;
    }
    if (flags[index] == locked) {
        return;
    }
    flags[index] = locked;
    state.locked_count += locked ? 1 : -1;
    if (state.locked_count < 0) {
        state.locked_count = 0;
    }
}

void CanvasModel::ClearLockedFlags() {
    auto &state = CurrentCanvasState();
    std::fill(state.is_locked.begin(), state.is_locked.end(), false);
    state.locked_count = 0;
}

int CanvasModel::GetLockedCount() const {
    return CurrentCanvasStateConst().locked_count;
}

int CanvasModel::GetLockedCountConst() const {
    return CurrentCanvasStateConst().locked_count;
}

int &CanvasModel::GetIterationRef() {
    return CurrentCanvasState().iteration;
}

int CanvasModel::GetIteration() const {
    return CurrentCanvasStateConst().iteration;
}

void CanvasModel::ResetIteration() {
    CurrentCanvasState().iteration = 0;
}

int CanvasModel::GetCurrentCanvasIndex() const {
    CurrentCanvasStateConst();
    return current_canvas_index_;
}

bool CanvasModel::SwitchCanvas(int index) {
    CurrentCanvasStateConst();
    if (index == current_canvas_index_ ||
        index < 0 ||
        index >= static_cast<int>(canvas_states_.size())) {
        return false;
    }
    current_canvas_index_ = index;
    return true;
}

size_t CanvasModel::GetCanvasStateCount() const {
    CurrentCanvasStateConst();
    return canvas_states_.size();
}

CanvasModel::PatternShapeType CanvasModel::GetCurrentPatternShape() const {
    return CurrentCanvasStateConst().pattern_shape;
}

bool CanvasModel::ChangePatternShape(PatternShapeType shape) {
    if (shape == GetCurrentPatternShape()) {
        return false;
    }

    auto &placed_polygons = GetPlacedPolygons();
    for (auto &poly : placed_polygons) {
        unplaced_polygons_.emplace_back(poly);
    }
    placed_polygons.clear();

    ClearLockedFlags();

    auto &state = CurrentCanvasState();
    ApplyPatternShape(state, shape);

    auto &opt_state = OptInfo::Current();
    opt_state.min_loss = state.cur_loss;
    opt_state.min_waste = state.cur_waste;
    opt_state.min_iter = 0;
    opt_state.min_unplaced_polygons.clear();
    opt_state.min_placed_polygons = placed_polygons;
    opt_state.min_canvas_placement_count = state.canvas_placement_count;
    opt_state.has_value = false;
    return true;
}

Grid<bool> &CanvasModel::GetPatternMask() {
    return CurrentCanvasState().pattern_mask;
}

const Grid<bool> &CanvasModel::GetPatternMaskConst() const {
    return CurrentCanvasStateConst().pattern_mask;
}

bool CanvasModel::AddCanvas() {
    canvas_states_.emplace_back();
    InitializeCanvasState(canvas_states_.back());
    OptInfo::Resize(static_cast<int>(canvas_states_.size()));
    auto &new_state = canvas_states_.back();
    auto &opt_state = OptInfo::At(static_cast<int>(canvas_states_.size()) - 1);
    opt_state.min_loss = new_state.cur_loss;
    opt_state.min_waste = new_state.cur_waste;
    opt_state.min_iter = 0;
    opt_state.min_unplaced_polygons = unplaced_polygons_;
    opt_state.min_placed_polygons = new_state.placed_polygons;
    opt_state.min_canvas_placement_count = new_state.canvas_placement_count;
    opt_state.has_value = false;

    current_canvas_index_ = static_cast<int>(canvas_states_.size()) - 1;
    return true;
}

bool CanvasModel::RemoveCanvas(int index) {
    if (canvas_states_.size() <= 1) {
        return false;
    }
    if (index < 0 || index >= static_cast<int>(canvas_states_.size())) {
        return false;
    }

    auto &state = canvas_states_[index];
    for (auto &poly : state.placed_polygons) {
        unplaced_polygons_.emplace_back(poly);
    }

    canvas_states_.erase(canvas_states_.begin() + index);
    OptInfo::Remove(index);

    if (current_canvas_index_ >= static_cast<int>(canvas_states_.size())) {
        current_canvas_index_ = static_cast<int>(canvas_states_.size()) - 1;
    }
    return true;
}

void CanvasModel::ApplyPatternShape(CanvasState &state, PatternShapeType shape) {
    int canvas_size = Config::GetCanvasSize();

    state.canvas_placement_count = Grid<int>(canvas_size, canvas_size, 0);

    state.pattern_shape = shape;
    state.pattern_mask = pattern_mask::BuildPatternMask(shape);
    state.valid_cell_count = pattern_mask::CountPatternMaskCells(state.pattern_mask);
    RefreshLayoutMetrics(state);
}

void CanvasModel::RefreshLayoutMetrics(CanvasState &state) {
    state.cur_waste = 0;
    state.cur_empty_count = state.valid_cell_count;
    state.cur_loss = static_cast<long long>(state.valid_cell_count) * Hyperparameter::np_pen;
}

std::mt19937 &CanvasModel::GetRngShapeGen() {
    return rng_shape_gen_;
}

std::mt19937 &CanvasModel::GetRngOpt() {
    return rng_opt_;
}

bool CanvasModel::IsAllowOverlap() const {
    return is_allow_overlap_;
}

void CanvasModel::SetAllowOverlap(bool allow) {
    is_allow_overlap_ = allow;
}

int CanvasModel::GetMaxIteration() const {
    return max_iteration_;
}

void CanvasModel::SetMaxIteration(int i) {
    max_iteration_ = i;
}
