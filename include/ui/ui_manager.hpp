#pragma once
#include <array>
#include <vector>

#include <Siv3D.hpp>

#include "core/canvas_state.hpp"
#include "core/polygon.hpp"
#include "ui/ui_placed_list.hpp"
#include "ui/ui_types.hpp"
#include "utils/config.hpp"

class CanvasModel;


class UIManager {
    public:
        using PatternShapeType = ui::PatternShapeType;
        using SelectType = ui::SelectType;
        using ScrapLoadMode = ui::ScrapLoadMode;
        using LayoutPattern = ui::LayoutPattern;

        explicit UIManager(CanvasModel &model);

        void SwitchCanvas(int index);
        void ChangePatternShape(PatternShapeType shape);

        void CreateUI();
    
        void Operations();
    
        inline void SetMouseInfo(const Point &p, const int w) {
            SetMousePos(p);
            SetMouseWheel(w);
        }
        inline Point GetMousePos() { return mouse_pos_; };
        inline int GetMouseWheel() { return mouse_wheel_; };
    
        bool InOperation();
  
    private:
        using Mode = CanvasMode;

        CanvasModel &model_;
        void InitializeWindow();
        void InitializeFonts();
        void InitializeModelState();
        void InitializeTempFolder();
        void InitializeUILayout();

        String GetPatternShapeLabel(PatternShapeType shape);
        void DrawPatternShapeDialog();

        Point mouse_pos_{};
        int mouse_wheel_ = 0; // not double; int
        inline void SetMousePos(const Point &p) { mouse_pos_ = p; };
        inline void SetMouseWheel(const double w) { mouse_wheel_ = w; };
        
        Rect canvas_outer_rect_;
        Rect layout_rect_;
        Rect unplaced_rect_;
        Rect control_rect_;
        Rect progress_bar_;
        Rect progress_bar_inner_;
        enum class FontId {
            Base,
            Small,
            Bold,
            Count,
        };
        std::array<Optional<Font>, static_cast<size_t>(FontId::Count)> fonts_{};
        Optional<Font> &FontSlot(FontId id);
        const Optional<Font> &FontSlot(FontId id) const;
        const Font &FontRef(FontId id) const;
        const Font &FontRefOr(FontId id, FontId fallback) const;

        Rect selection_priority_box_;
        Rect iteration_box_;
        Rect change_rate_box_;
        Rect reg_shape_priority_box_;
    
        std::vector<Rect> canvas_selector_buttons_;

        enum class ButtonId {
            Delete,
            Initialize,
            Optimize,
            LoadImage,
            CutScraps,
            Reset,
            CanvasSave,
            CanvasFront,
            CanvasBack,
            ResetWeights,
            PatternShape,
            AddCanvas,
            RemoveCanvas,
            ScrapPreset,
            Count,
        };
        std::array<Rect, static_cast<size_t>(ButtonId::Count)> button_rects_{};
        Rect& ButtonRect(ButtonId id);

        bool Button(const Rect &rect, const Font &font, const String &text,
            const ColorF &font_color_enabled, const ColorF &font_color_disabled,
            const ColorF &box_color_enabled, const ColorF &box_color_disabled,
            bool enabled);
    
        bool is_dragging_ = false;
        int dragging_unplaced_index_ = -1;
        MyPolygon temp_dragged_polygon_;
    
        bool is_initializing_ = false;
        bool is_optimizing_ = false;
        bool is_saving_ = false;
        enum class SaveCaptureState {
            Idle,
            Requested,
            Ready,
        };
        SaveCaptureState save_capture_state_ = SaveCaptureState::Idle;
        bool is_loaded_ = false;
        bool is_layout_set_ = false;
    
        FilePath screenshot_path_;
        int screenshot_index_ = 0;
    
        SelectType selected_type_ = SelectType::None;
        int selected_index_ = -1;
    
        double placed_wheel_rotate_rad_ = 0.0; // Rotation of placed scrap (in radians)

        int cur_unplaced_scroll_offset_ = 0;
        int unplaced_scroll_ = 0; // Mouse wheel scroll speed for unplaced list

        void CutScrap();
        bool LaunchMeasureScrap(const String& mode = U"", const int index = -1);
        
        String tempFolderPath_;
        String lastEditedScrapFolder_;
        bool is_pattern_dialog_open_ = false;
        bool pattern_dialog_just_opened_ = false;
    
        void DrawFrames();
        void DrawButtons();
        void DrawSliders();
        void DrawUnplaced();
        void DrawDrag();
        void DrawCanvas();
        void DrawCheckBoxes();
        void DrawCanvasPlacedList();
        void UpdateCanvasSelectionAndRotation();
        void UpdateCanvasPlacedList();
        void UpdateUnplacedInteraction();
        void UpdateDrag();
        PlacedListWidget placed_list_;

        void DrawIterationSlider(int control_ui_x, int control_ui_y, int button_margin, int header_height);
        void DrawChangeRateSlider(int control_ui_x, int control_ui_y, int button_margin, int header_height);
        void DrawSelectionPrioritySlider(int unplaced_ui_x, int unplaced_ui_width, int control_ui_y, int button_margin, int header_height);
        void DrawRegularShapeSlider(int unplaced_ui_x, int unplaced_ui_width, int control_ui_y, int button_margin, int header_height);
    
        void CanvasInitialize();
        void CanvasOptimize();
        void CanvasReset();
        void CanvasSave();
        void FinalizeOptimization(bool applyBest = true);
        void AddCanvas();
        void RemoveCurrentCanvas();

    
        void Initialize();
        ScrapLoadMode scrap_load_mode_ = ScrapLoadMode::Preset;
        ScrapLoadMode NextScrapLoadMode(ScrapLoadMode mode);
        String GetScrapModeLabel();
        String GetLoadButtonLabel();
  };
