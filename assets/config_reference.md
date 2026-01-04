# Config Reference

This project uses two config files:

- `assets/config.json` (app/runtime settings)
- `assets/pattern_shape.json` (pattern mask shapes)

## assets/config.json

### seeds
- `seed0`: RNG seed for optimization.
- `seed1`: RNG seed for polygon generation.

### ui_size
All values are pixels in the UI.
- `window.width`, `window.height`, `window.title`: window size and title.
- `canvas_ui.x`, `canvas_ui.y`, `canvas_ui.size`: canvas viewport position/size.
- `unplaced_ui.x`, `unplaced_ui.y`, `unplaced_ui.width`, `unplaced_ui.height`: unplaced list area.
- `control_ui.x`, `control_ui.y`, `control_ui.width`, `control_ui.height`: control panel area.

### paths
All paths are resolved relative to the working directory (typically `build/`).
- `preset_outputs_roots`: root directory for preset scraps.
- `cropper_temp_dir`: temp directory for cropper edit mode.
- `cropper_outputs_dir`: output directory for cropper new mode.
- `cropper_script_path`: path to the cropper entry script.
- `layout_exports_dir`: output directory for Save Layout exports.

### generate_polygon
All numeric values are in layout units.
- `count`: number of polygons to generate.
- `is_large`: prefer larger polygons.
- `is_rectangle`: generate rectangles instead of random polygons.
- `v_min`, `v_max`: vertex count range for random polygons.
- `r_mean`, `r_var`: mean/variance of random polygon radius.
- `rect_large_min`, `rect_large_max`: rectangle size range for large mode.
- `rect_small_min`, `rect_small_max`: rectangle size range for small mode.
- `min_extent_large`, `min_extent_small`: minimum bounding size for random polygons.

### font
- `font_base.size`: font size in pixels.
- `font_base.type`: font file path.

### unplaced_list
- `icon_size`: icon size in pixels.
- `margin`: spacing in pixels.

### input
- `placed_wheel_rotate_deg`: rotation step in degrees (converted to radians internally).
- `unplaced_scroll_step`: scroll step in pixels.

### button
- `width`, `height`, `margin`: button size/margin in pixels.

### canvas
All numeric values are in layout units.
- `margin`: outer margin around the layout area.
- `layout_size`: size of the inner layout area.
- `num_canvases`: number of layouts.
- `max_polygons`: capacity limit for scraps.

### startup_cleanup
- `measure_outputs`: delete cropper outputs on startup.
- `measure_temp`: delete cropper temp outputs on startup.

### hyperparameter
Optimization parameters.
- `maxiter`: max iterations.
- `steps`: steps per frame.
- `P`, `noP`: operation selection weights.
- `np_pen`, `ol_pen`, `or_pen`: penalties for empty/overlap/outside.
- `sttmp`, `entmp`: annealing temperature start/end.

### polygon_config
Polygon rendering/selection parameters.
- `rect_similarity_calc_step`: angle step in degrees for similarity calc.
- `texture_scale_divisor`: divisor for texture display scaling.
- `seam_allowance`: shrink amount in layout units (applied before optimization).


## assets/pattern_shape.json

Pattern shape parameters. Shapes are defined in normalized coordinates (0.0–1.0) and
scaled by `layout_size`. These shapes are experimental and can be freely edited or extended.

- `layout_size`: scaling reference for pattern shapes. Set this to match `canvas.layout_size`
  unless you intentionally want a different scale.

### pattern_shapes
- `square.min`, `square.max`: normalized bounds (0.0–1.0). `eps` is in layout units.
- `triangle.a`, `triangle.b`, `triangle.c`: normalized vertices.
- `donut.center`, `donut.outer`, `donut.inner`: normalized ring parameters.
- `hexagon.center`, `hexagon.radius`: normalized center and radius.
- `ellipse.center`, `ellipse.a`, `ellipse.b`: normalized ellipse parameters.
- `polygon.vertices`: normalized vertices for an arbitrary polygon.

