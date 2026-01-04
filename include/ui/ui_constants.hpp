#pragma once

#include <Siv3D.hpp>


namespace ui {

inline constexpr s3d::Color WindowColor = s3d::Palette::Dimgrey;
inline constexpr double PatternEps = 1e-6;
inline constexpr int UnplacedWeightLabelHeight = 20;

// Layout selector button dimensions
inline constexpr int LayoutSelectorButtonWidth = 70;
inline constexpr int LayoutSelectorButtonHeight = 70;
inline constexpr int LayoutSelectorMargin = 10;

// Placed list dimensions
inline constexpr int PlacedListWidth = 270;
inline constexpr int PlacedListItemHeight = 24;
inline constexpr int PlacedListMaxVisibleItems = 6;
inline constexpr int PlacedListPadding = 5;
inline constexpr int PlacedListTitleHeight = 30;

// Auto-scroll margin
inline constexpr int AutoScrollMargin = 16;

// Drag handle dimensions
inline constexpr int DragHandleSize = 20;
inline constexpr int DragHandleLineSpacing = 5;
inline constexpr int DragHandleLineCount = 3;

// Checkbox dimensions
inline constexpr int CheckboxSize = 20;
inline constexpr int CheckboxTextOffset = 25;

// Button spacing
inline constexpr int ButtonSpacing = 8;

// Scrollbar dimensions
inline constexpr int ScrollbarWidth = 10;
inline constexpr int ScrollbarMinThumbHeight = 20;

} // namespace ui
