#pragma once
#include <random>
#include <vector>

#include <Siv3D.hpp>

#include "core/canvas_state.hpp"
#include "core/polygon.hpp"
#include "ui/ui_types.hpp"


class CanvasModel {
    public:
        using PatternShapeType = ui::PatternShapeType;
        using LayoutPattern = ui::LayoutPattern;

        static CanvasModel &Get();

        Grid<int> &GetCanvasPlacementCount();
        const Grid<int> &GetCanvasPlacementCountConst() const;
        std::vector<MyPolygon> &GetPlacedPolygons();
        const std::vector<MyPolygon> &GetPlacedPolygonsConst() const;
        std::vector<MyPolygon> &GetUnplacedPolygons();
        const std::vector<MyPolygon> &GetUnplacedPolygonsConst() const;
        int GetUnplacedPolygonNum() const;
        void LoadAllScraps();
        void LoadNewScraps();
        void GeneratePolygons(int num = 1);
        bool DeleteUnplacedScrapAt(int index);

        long long &GetCurLoss();
        int GetCurWaste() const;
        void SetCurWaste(int waste);
        int &GetCurEmptyCount();
        const std::vector<bool> &GetLockedFlagsConst() const;
        bool IsLocked(int index) const;
        void SetLocked(int index, bool locked);
        void ClearLockedFlags();
        int GetLockedCount() const;
        int GetLockedCountConst() const;
        int &GetIterationRef();
        int GetIteration() const;
        void ResetIteration();

        int GetCurrentCanvasIndex() const;
        bool SwitchCanvas(int index);
        size_t GetCanvasStateCount() const;

        PatternShapeType GetCurrentPatternShape() const;
        bool ChangePatternShape(PatternShapeType shape);
        Grid<bool> &GetPatternMask();
        const Grid<bool> &GetPatternMaskConst() const;

        void InitializeCanvasStates(int numLayouts);
        void InitializeCanvasState(CanvasState &state);
        bool AddCanvas();
        bool RemoveCanvas(int index);

        void ApplyPatternShape(CanvasState &state, PatternShapeType shape);
        void RefreshLayoutMetrics(CanvasState &state);

        void CaptureCancelSnapshot();
        void RestoreCancelSnapshot();
        void ClearCancelSnapshot();
        bool HasCancelSnapshot() const;

        bool RunInitializationStep();
        bool RunOptimizationStep();

        std::mt19937 &GetRngShapeGen();
        std::mt19937 &GetRngOpt();

        bool IsAllowOverlap() const;
        void SetAllowOverlap(bool allow);

        int GetMaxIteration() const;
        void SetMaxIteration(int i);
        CanvasState &CurrentCanvasState();
        const CanvasState &CurrentCanvasStateConst() const;

    private:
        CanvasModel() = default;

        std::vector<MyPolygon> unplaced_polygons_;
        std::vector<CanvasState> canvas_states_;
        int current_canvas_index_ = 0;

        std::vector<LayoutPattern> layout_patterns_;
        int current_pattern_index_ = 0;
        bool is_patterns_loaded_ = false;

        // rng_opt: optimization/placement; rng_shape_gen: shape/scrap generation. 
        std::mt19937 rng_opt_;
        std::mt19937 rng_shape_gen_;

        bool is_allow_overlap_ = false;
        int max_iteration_ = 30000;

        // Snapshot for cancel: original canvas state at optimization start.
        bool cancel_snapshot_valid_ = false;
        std::vector<MyPolygon> cancel_snapshot_placed_;
        std::vector<MyPolygon> cancel_snapshot_unplaced_;
        Grid<int> cancel_snapshot_grid_;
        long long cancel_snapshot_loss_ = 0;
        int cancel_snapshot_waste_ = 0;
        int cancel_snapshot_empty_count_ = 0;
};
