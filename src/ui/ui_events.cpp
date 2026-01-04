#include "ui/ui_manager.hpp"

#include <algorithm>

using namespace s3d;

void UIManager::CreateUI() {
    if (save_capture_state_ == SaveCaptureState::Requested) {
        save_capture_state_ = SaveCaptureState::Ready;
    }
    MyPolygon::SetDrawOriginal(save_capture_state_ == SaveCaptureState::Ready);

    // Mind the draw order（z-buffer）
    DrawFrames();
    DrawButtons();
    DrawCanvasPlacedList();
    DrawSliders();
    if (is_pattern_dialog_open_) {
        DrawPatternShapeDialog();
    }
}

void UIManager::Operations() {
    const auto update_input = [this]() {
        UpdateCanvasSelectionAndRotation();
        UpdateCanvasPlacedList();
        UpdateUnplacedInteraction();
        if (!InOperation()) {
            UpdateDrag();
        }
    };

    const auto update_optimization = [this]() {
        if (is_initializing_) {
            bool is_fin = model_.RunInitializationStep();
            if (is_fin) {
                is_initializing_ = false;
                CanvasOptimize();
            }
            return;
        }

        if (!is_optimizing_) {
            return;
        }

        for (int i = 0; i < Config::GetOptSteps(); ++i) {
            if (!is_optimizing_) {
                break;
            }
            if (model_.RunOptimizationStep()) {
                FinalizeOptimization(true);
                break;
            }
        }
    };

    update_input();
    update_optimization();
}

void UIManager::SwitchCanvas(int index) {
    if (InOperation()) {
        return;
    }
    if (!model_.SwitchCanvas(index)) {
        return;
    }
    
    selected_type_ = UIManager::SelectType::None;
    selected_index_ = -1;
    is_dragging_ = false;
    is_pattern_dialog_open_ = false;
    pattern_dialog_just_opened_ = false;
}

void UIManager::AddCanvas() {
    if (InOperation()) {
        return;
    }

    is_pattern_dialog_open_ = false;
    pattern_dialog_just_opened_ = false;

    model_.AddCanvas();
    selected_type_ = SelectType::None;
    selected_index_ = -1;
}

void UIManager::RemoveCurrentCanvas() {
    if (InOperation()) {
        return;
    }
    if (!model_.RemoveCanvas(model_.GetCurrentCanvasIndex())) {
        return;
    }

    selected_type_ = SelectType::None;
    selected_index_ = -1;
    is_pattern_dialog_open_ = false;
    pattern_dialog_just_opened_ = false;
}
