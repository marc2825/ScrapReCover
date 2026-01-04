#include "ui/ui_manager.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>

#include <nlohmann/json.hpp>

#include "algorithm/eval_functions.hpp"
#include "algorithm/optimizer.hpp"
#include "core/canvas_model.hpp"
#include "ui/ui_constants.hpp"
#include "utils/layout_utils.hpp"
#include "utils/polygon_generator.hpp"


Rect& UIManager::ButtonRect(ButtonId id) {
    return button_rects_[static_cast<size_t>(id)];
}

Optional<Font> &UIManager::FontSlot(FontId id) {
    return fonts_[static_cast<size_t>(id)];
}

const Optional<Font> &UIManager::FontSlot(FontId id) const {
    return fonts_[static_cast<size_t>(id)];
}

const Font &UIManager::FontRef(FontId id) const {
    const auto &slot = FontSlot(id);
    if (slot.has_value()) {
        return slot.value();
    }
    return FontSlot(FontId::Base).value();
}

const Font &UIManager::FontRefOr(FontId id, FontId fallback) const {
    const auto &slot = FontSlot(id);
    if (slot.has_value()) {
        return slot.value();
    }
    return FontRef(fallback);
}

UIManager::UIManager(CanvasModel &model) : model_(model) {
    InitializeWindow();
    InitializeFonts();
    InitializeModelState();
    InitializeTempFolder();
    InitializeUILayout();
}

bool UIManager::InOperation() { return is_initializing_ || is_optimizing_; };

void UIManager::InitializeWindow() {
    int window_width = Config::GetWindowWidth();
    int window_height = Config::GetWindowHeight();
    String window_title = Config::GetTitle();

    Window::Resize(window_width, window_height);
    Window::SetTitle(window_title);
    Scene::SetBackground(ui::WindowColor);
}

void UIManager::InitializeFonts() {
    int font_base_size = Config::GetFontBaseSize();
    String font_base_type = Config::GetFontBaseType(); // TODO: need fix?
    FontSlot(FontId::Base).emplace(Font(FontMethod::MSDF, font_base_size));
    FontSlot(FontId::Bold).emplace(Font(15, Typeface::Bold));
    FontSlot(FontId::Small).emplace(Font(FontMethod::MSDF, static_cast<int>(font_base_size * 0.7)));
}

void UIManager::InitializeModelState() {
    model_.InitializeCanvasStates(Config::GetNumCanvases());
    is_pattern_dialog_open_ = false;
    pattern_dialog_just_opened_ = false;
    scrap_load_mode_ = ScrapLoadMode::Preset;
    placed_wheel_rotate_rad_ = Config::GetPlacedWheelRotateRad();
    unplaced_scroll_ = Config::GetUnplacedScrollStep();
}

void UIManager::InitializeTempFolder() {
    const FilePath outputsFolder = Config::GetMeasureOutputsDir();
    if (Config::GetCleanupMeasureOutputsOnStartup()) {
        if (FileSystem::IsDirectory(outputsFolder)) {
            for (const auto& item : FileSystem::DirectoryContents(outputsFolder, Recursive::Yes)) {
                FileSystem::Remove(item);
            }
            FileSystem::Remove(outputsFolder);
        }
        FileSystem::CreateDirectories(outputsFolder);
    }

    const FilePath tempFolder = Config::GetMeasureTempDir();
    if (Config::GetCleanupMeasureTempOnStartup()) {
        if (FileSystem::IsDirectory(tempFolder)) {
            for (const auto& item : FileSystem::DirectoryContents(tempFolder, Recursive::Yes)) {
                FileSystem::Remove(item);
            }
            FileSystem::Remove(tempFolder);
        }
        FileSystem::CreateDirectories(tempFolder);
    }
}

void UIManager::InitializeUILayout() {
    // Top-left: canvas area (Layout workspace)
    int canvas_ui_x = Config::GetCanvasUIX();
    int canvas_ui_y = Config::GetCanvasUIY();
    int canvas_ui_size = Config::GetCanvasUISize();
    int canvas_margin = Config::GetCanvasMargin();
    double canvas_ui_ratio = Config::GetCanvasUIRatio();
    int layout_size = Config::GetLayoutSize();

    model_.SetAllowOverlap(false);
    canvas_outer_rect_ = Rect(canvas_ui_x, canvas_ui_y, canvas_ui_size, canvas_ui_size);
    layout_rect_ =
        Rect(canvas_ui_x + canvas_margin * canvas_ui_ratio, canvas_ui_y + canvas_margin * canvas_ui_ratio,
             layout_size * canvas_ui_ratio, layout_size * canvas_ui_ratio);

    const int progress_margin_x = 10;
    const int progress_margin_bottom = 5;
    const int progress_height = 20;
    double progress = static_cast<double>(model_.GetIteration()) / model_.GetMaxIteration();
    progress_bar_ =
        Rect(canvas_ui_x + progress_margin_x,
             canvas_ui_y + canvas_ui_size - progress_margin_bottom - progress_height,
             canvas_ui_size - progress_margin_x * 2, progress_height);


    // Top-right: unplaced polygon list (Available scraps list)
    int unplaced_ui_x = Config::GetUnplacedUIX();
    int unplaced_ui_y = Config::GetUnplacedUIY();
    int unplaced_ui_width = Config::GetUnplacedUIWidth();
    int unplaced_ui_height = Config::GetUnplacedUIHeight();

    unplaced_rect_ = Rect(unplaced_ui_x, unplaced_ui_y, unplaced_ui_width, unplaced_ui_height);


    // Top-center: layout selector buttons.
    const int selector_button_width = ui::LayoutSelectorButtonWidth;
    const int selector_button_height = ui::LayoutSelectorButtonHeight;
    const int selector_margin = ui::LayoutSelectorMargin;
    const int selector_start_x = canvas_ui_x + canvas_ui_size + selector_margin;
    const int selector_start_y = canvas_ui_y + 20;
    const int selector_stride_y = selector_button_height + selector_margin;
    
    canvas_selector_buttons_.clear();
    int num_canvas = static_cast<int>(model_.GetCanvasStateCount());
    for (int i = 0; i < num_canvas; ++i) {
        canvas_selector_buttons_.emplace_back(
            Rect(selector_start_x, selector_start_y + i * selector_stride_y,
                 selector_button_width, selector_button_height));
    }

    
    // Bottom-left: Layout control panel
    // Bottom-right: Scrap control panel
    int control_ui_x = Config::GetControlUIX();
    int control_ui_y = Config::GetControlUIY();
    int control_ui_width = Config::GetControlUIWidth();
    int control_ui_height = Config::GetControlUIHeight();

    control_rect_ = Rect(control_ui_x, control_ui_y, control_ui_width, control_ui_height);

    int button_margin = Config::GetButtonMargin();
    int button_width = Config::GetButtonWidth();
    int button_height = Config::GetButtonHeight();
    const int button_stride_x = button_width + button_margin;
    const int button_stride_y = button_height + button_margin;
    const int button_row_index = 2;
    const int button_row_y = control_ui_y + button_margin + button_stride_y * button_row_index;
    const int left_group_x = control_ui_x + button_margin;
    const int right_group_x = unplaced_ui_x + button_margin;

    // controls (left)
    ButtonRect(ButtonId::CanvasSave) = Rect(left_group_x, button_row_y,
        button_width, button_height);

    ButtonRect(ButtonId::Optimize) = Rect(left_group_x + button_stride_x * 1, button_row_y,
        button_width, button_height);

    ButtonRect(ButtonId::Reset) = Rect(left_group_x + button_stride_x * 2, button_row_y,
        button_width, button_height);

    // controls (right)
    ButtonRect(ButtonId::LoadImage) = Rect(right_group_x, button_row_y,
        button_width, button_height);

    ButtonRect(ButtonId::CutScraps) = Rect(right_group_x + button_stride_x * 1, button_row_y,
        button_width, button_height);

    ButtonRect(ButtonId::Delete) = Rect(right_group_x + button_stride_x * 2, button_row_y,
        button_width, button_height);

    const int reset_weights_offset_x = 100;
    const int reset_weights_offset_y = 10;
    const int reset_weights_width = button_width * 2 / 3 - 5;
    const int reset_weights_height = button_height * 2 - 20;
    ButtonRect(ButtonId::ResetWeights) = Rect(
        unplaced_ui_x + unplaced_ui_width - reset_weights_offset_x,
        control_ui_y + button_margin + reset_weights_offset_y,
        reset_weights_width, reset_weights_height
    );
}
