from dataclasses import dataclass
import json
import os
from pathlib import Path
from typing import List, Optional, Tuple

import tkinter as tk
from tkinter import filedialog, messagebox
from PIL import Image, ImageTk, UnidentifiedImageError

from geometry_helpers import centroid, flatten_points, scale_points
from image_handler import ImageHandler

# Please ensure the camera is parallel to the surface.

DEFAULT_CANVAS_SIZE = 500
MAX_DISPLAY_SIZE_DEFAULT = 600 # SCALE_DIVISOR is applied after the scale down of the image 
POINT_RADIUS = 3
PREVIEW_LINE_WIDTH = 2
SCALE_DIVISOR_DEFAULT = 11.0 # Set magnification ratio based on predefined camera-scrap distance (configurable via config.json).

TEMP_POINTS_FILENAME = "polygon_points.json"
TEMP_TEXTURE_FILENAME = "polygon_texture.png"

PANEL_PADDING = 5
TITLE_PADDING_Y = (0, 10)
LIST_LABEL_PADDING_Y = (40, 0)
BUTTON_WIDTH = 7
LISTBOX_HEIGHT = 15
LOAD_BUTTON_PAD_TOP = 110

TITLE_FONT = ("Arial", 14, "bold")
LABEL_FONT = ("Arial", 12)
BUTTON_FONT = ("Arial", 11, "bold")

COLOR_POINT_DEFAULT = "red"
COLOR_POINT_SELECTED = "green"
COLOR_LINE = "red"
COLOR_IMAGE_BG = "gray"

IMAGE_POINT_TAG = "image_point"
IMAGE_OUTLINE_TAG = "image_outline"
IMAGE_DASH_TAG = "image_outline_dash"

PROJECT_ROOT_DIR = str(Path(__file__).resolve().parents[3])

PointList = List[Tuple[float, float]]


@dataclass
class AppConfig:
    mode: str
    index: int
    temp_dir: str
    outputs_dir: str
    scale_divisor: float
    max_display_size: int


@dataclass
class SavePayload:
    scaled_points: PointList
    texture: Image.Image


class PolygonModel:
    """Hold polygon data, image data, and save logic."""

    def __init__(self, config: AppConfig) -> None:
        self.config = config
        self.image_handler = ImageHandler(config.max_display_size)
        self.points: PointList = []
        self.original_points: Optional[PointList] = None
        self.original_centroid: Optional[Tuple[float, float]] = None
        self.history: List[PointList] = []
        self.saved = False

    def load_existing(self) -> None:
        points_path = os.path.join(self.config.temp_dir, TEMP_POINTS_FILENAME)
        texture_path = os.path.join(self.config.temp_dir, TEMP_TEXTURE_FILENAME)

        raw_points: Optional[PointList] = None
        if os.path.exists(points_path):
            with open(points_path, 'r', encoding='utf-8-sig') as f:
                data = json.load(f)
            raw_points = self._parse_points(data)

        if os.path.exists(texture_path):
            self.image_handler.load(texture_path)

        if raw_points is not None:
            if self.image_handler.has_image():
                display_size = self.image_handler.get_display_size()
                raw_points = self._normalize_points_to_image(raw_points, display_size)
                self.points = raw_points
            else:
                self.points = raw_points
            self.original_points = list(self.points)
            self.original_centroid = centroid(self.points) if self.points else None
        self.history = []

    def load_image(self, path: str) -> None:
        self.image_handler.load(path)
        self.points = []
        self.original_points = None
        self.original_centroid = None
        self.history = []
        self.saved = False

    def add_point(self, point: Tuple[float, float]) -> None:
        self._push_history()
        self.points.append(point)
        self.saved = False

    def remove_last_point(self) -> None:
        if self.points:
            self._push_history()
            self.points.pop()
            self.saved = False

    def remove_point_at(self, index: int) -> None:
        if 0 <= index < len(self.points):
            self._push_history()
            self.points.pop(index)
            self.saved = False

    def undo_last_change(self) -> bool:
        if not self.history:
            return False
        self.points = self.history.pop()
        self.saved = False
        return True

    def _push_history(self) -> None:
        self.history.append(list(self.points))

    def get_display_points(self) -> PointList:
        return self.points

    def get_source_points(self) -> PointList:
        return self.points

    def get_scaled_points(self) -> PointList:
        if not self.points:
            return []
        if self.config.mode == "edit" and self.original_centroid is not None:
            center = self.original_centroid
        else:
            center = centroid(self.points)
        scale_factor = 1.0 / self.config.scale_divisor
        return scale_points(self.points, scale_factor, center)

    def save(self) -> str:
        self._validate_save()
        save_dir = self._resolve_save_dir()
        payload = self._build_save_payload()
        self._write_points_json(save_dir, payload.scaled_points)
        self._write_texture(save_dir, payload.texture)
        self.saved = True
        return save_dir

    def cleanup_edit_temp_files(self) -> None:
        for filename in (TEMP_POINTS_FILENAME, TEMP_TEXTURE_FILENAME):
            path = os.path.join(self.config.temp_dir, filename)
            if os.path.exists(path):
                try:
                    os.remove(path)
                except OSError:
                    pass

    def _validate_save(self) -> None:
        if not self.image_handler.has_image():
            raise ValueError("No image has been loaded")
        if len(self.points) < 3:
            raise ValueError("A polygon requires at least 3 vertices")
        if self.config.scale_divisor <= 0.0:
            raise ValueError("Scale divisor must be positive")

    def _resolve_save_dir(self) -> str:
        if self.config.mode == "edit":
            save_dir = self.config.temp_dir
            os.makedirs(save_dir, exist_ok=True)
            return save_dir
        return self._get_unique_save_dir(self.config.outputs_dir)

    def _build_save_payload(self) -> SavePayload:
        source_points = self.get_source_points()
        if self.config.mode == "edit" and self.original_centroid is not None:
            center = self.original_centroid
        else:
            center = centroid(source_points)
        scale_factor = 1.0 / self.config.scale_divisor
        scaled_points = scale_points(source_points, scale_factor, center)
        texture = self.image_handler.make_texture(source_points)
        return SavePayload(scaled_points=scaled_points, texture=texture)

    def _write_points_json(self, save_dir: str, points: PointList) -> None:
        points_path = os.path.join(save_dir, TEMP_POINTS_FILENAME)
        with open(points_path, 'w', encoding='utf-8') as f:
            json.dump(points, f, ensure_ascii=False, indent=2)

    def _write_texture(self, save_dir: str, texture: Image.Image) -> None:
        texture_path = os.path.join(save_dir, TEMP_TEXTURE_FILENAME)
        texture.save(texture_path, "PNG")

    @staticmethod
    def _parse_points(data: object) -> PointList:
        def parse_pair(entry: object, index: int) -> Tuple[float, float]:
            if isinstance(entry, (list, tuple)) and len(entry) == 2:
                try:
                    return (float(entry[0]), float(entry[1]))
                except (TypeError, ValueError) as exc:
                    raise ValueError(f"Invalid point values at index {index}") from exc
            if isinstance(entry, dict):
                if "x" in entry and "y" in entry:
                    try:
                        return (float(entry["x"]), float(entry["y"]))
                    except (TypeError, ValueError) as exc:
                        raise ValueError(f"Invalid point values at index {index}") from exc
                if "first" in entry and "second" in entry:
                    try:
                        return (float(entry["first"]), float(entry["second"]))
                    except (TypeError, ValueError) as exc:
                        raise ValueError(f"Invalid point values at index {index}") from exc
            raise ValueError(f"Invalid point format at index {index}")

        if isinstance(data, dict):
            if "points" in data:
                data = data["points"]
            else:
                keys = list(data.keys())
                numeric_keys = [k for k in keys if isinstance(k, str) and k.isdigit()]
                if numeric_keys and len(numeric_keys) == len(keys):
                    data = [data[k] for k in sorted(numeric_keys, key=int)]
                else:
                    data = list(data.values())

        if not isinstance(data, list):
            raise ValueError("Invalid points data")

        if not data:
            return []

        if all(isinstance(entry, (int, float)) for entry in data):
            if len(data) % 2 != 0:
                raise ValueError("Invalid flat points list length")
            return [(float(data[i]), float(data[i + 1])) for i in range(0, len(data), 2)]

        points: PointList = []
        for index, entry in enumerate(data):
            points.append(parse_pair(entry, index))
        return points

    @staticmethod
    def _get_unique_save_dir(base_dir: str) -> str:
        os.makedirs(base_dir, exist_ok=True)
        index = 0
        while True:
            save_dir = os.path.join(base_dir, f"polygon_{index}")
            if not os.path.exists(save_dir):
                os.makedirs(save_dir)
                return save_dir
            index += 1

    @staticmethod
    def _normalize_points_to_image(points: PointList,
                                   image_size: Tuple[int, int]) -> PointList:
        if not points:
            return points
        image_width, image_height = image_size
        if image_width <= 0 or image_height <= 0:
            return points

        min_x = min(p[0] for p in points)
        max_x = max(p[0] for p in points)
        min_y = min(p[1] for p in points)
        max_y = max(p[1] for p in points)
        span_x = max_x - min_x
        span_y = max_y - min_y
        if span_x <= 0.0 or span_y <= 0.0:
            return points

        shift_x = -min_x if min_x < 0.0 else 0.0
        shift_y = -min_y if min_y < 0.0 else 0.0
        shifted = [(x + shift_x, y + shift_y) for x, y in points]

        max_x_shifted = max(p[0] for p in shifted)
        max_y_shifted = max(p[1] for p in shifted)
        if max_x_shifted <= image_width and max_y_shifted <= image_height:
            return shifted if (shift_x != 0.0 or shift_y != 0.0) else points

        scale_x = image_width / max_x_shifted
        scale_y = image_height / max_y_shifted
        scale = min(1.0, scale_x, scale_y)
        if scale >= 1.0:
            return shifted if (shift_x != 0.0 or shift_y != 0.0) else points

        return [(x * scale, y * scale) for x, y in shifted]


class PolygonView:
    """Render the UI."""

    def __init__(self, master: tk.Tk) -> None:
        self.master = master
        self.tk_image: Optional[ImageTk.PhotoImage] = None
        self._suppress_listbox_event = False
        self._create_widgets()

    def _create_widgets(self) -> None:
        self._create_image_panel()
        self._create_list_panel()
        self._create_controls_panel()

    def _create_image_panel(self) -> None:
        self.frame_left = tk.Frame(self.master)
        self.frame_left.pack(side=tk.LEFT, padx=PANEL_PADDING, pady=PANEL_PADDING)
        self.title_label_left = tk.Label(self.frame_left, text="Scrap Image", font=TITLE_FONT)
        self.title_label_left.pack(side="top", pady=TITLE_PADDING_Y)

        self.canvas_image = tk.Canvas(self.frame_left, width=DEFAULT_CANVAS_SIZE,
                                      height=DEFAULT_CANVAS_SIZE, bg=COLOR_IMAGE_BG)
        self.canvas_image.pack()

    def _create_list_panel(self) -> None:
        self.info_frame = tk.Frame(self.master)
        self.info_frame.pack()

        self.list_frame = tk.Frame(self.info_frame)
        self.list_frame.pack(side=tk.TOP, fill=tk.Y)

        self.coords_label = tk.Label(self.list_frame, text="Vertex Coordinates", font=LABEL_FONT)
        self.coords_label.pack(side=tk.TOP, pady=LIST_LABEL_PADDING_Y)

        self.vertex_button_frame = tk.Frame(self.list_frame)
        self.vertex_button_frame.pack(side=tk.BOTTOM, fill=tk.X)
        self.undo_button = tk.Button(self.vertex_button_frame, text="Undo", width=BUTTON_WIDTH)
        self.undo_button.pack(side=tk.LEFT, padx=PANEL_PADDING)

        self.delete_button = tk.Button(self.vertex_button_frame, text="Delete", width=BUTTON_WIDTH)
        self.delete_button.pack(side=tk.RIGHT, padx=PANEL_PADDING)

        self.scrollbar = tk.Scrollbar(self.list_frame, orient=tk.VERTICAL)
        self.coords_listbox = tk.Listbox(self.list_frame, height=LISTBOX_HEIGHT,
                                         yscrollcommand=self.scrollbar.set)
        self.coords_listbox.pack(side=tk.LEFT, fill=tk.X, expand=True)

        self.scrollbar.config(command=self.coords_listbox.yview)
        self.scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

    def _create_controls_panel(self) -> None:
        self.bottom_frame = tk.Frame(self.info_frame)
        self.bottom_frame.pack(side=tk.BOTTOM, fill=tk.X)

        self.save_button = tk.Button(self.bottom_frame, text="Save Polygon", font=BUTTON_FONT)
        self.save_button.pack(side=tk.BOTTOM, fill=tk.X, padx=PANEL_PADDING,
                              anchor=tk.S, expand=True)

        self.load_button = tk.Button(self.bottom_frame, text="Load Image", font=BUTTON_FONT)
        self.load_button.pack(side=tk.BOTTOM, fill=tk.X, padx=PANEL_PADDING,
                              pady=(LOAD_BUTTON_PAD_TOP, 0), anchor=tk.S, expand=True)

    def set_title(self, title: str) -> None:
        self.master.title(title)

    def set_image(self, image_handler: ImageHandler) -> None:
        if image_handler.display_image is None:
            return

        self.tk_image = ImageTk.PhotoImage(image_handler.display_image)
        width, height = image_handler.get_display_size()
        self.canvas_image.config(width=width, height=height)

        self.canvas_image.delete("all")
        self.canvas_image.create_image(0, 0, anchor=tk.NW, image=self.tk_image)

    def update_points(self,
                      list_points: PointList,
                      display_points: PointList,
                      selected_index: Optional[int]) -> None:
        self._update_listbox(list_points, selected_index)
        self._draw_outline_on_image(display_points)
        self._draw_points_on_image(display_points, selected_index)

    def _update_listbox(self,
                        points: PointList,
                        selected_index: Optional[int]) -> None:
        self._suppress_listbox_event = True
        self.coords_listbox.delete(0, tk.END)
        for x, y in points:
            self.coords_listbox.insert(tk.END, f"({int(x)}, {int(y)})")
        if selected_index is not None and 0 <= selected_index < len(points):
            self.coords_listbox.selection_set(selected_index)
            self.coords_listbox.activate(selected_index)
            self.coords_listbox.see(selected_index)
        self._suppress_listbox_event = False

    def _draw_points_on_image(self,
                              points: PointList,
                              selected_index: Optional[int]) -> None:
        self.canvas_image.delete(IMAGE_POINT_TAG)
        for i, (x, y) in enumerate(points):
            color = COLOR_POINT_SELECTED if selected_index == i else COLOR_POINT_DEFAULT
            self.canvas_image.create_oval(x - POINT_RADIUS, y - POINT_RADIUS,
                                          x + POINT_RADIUS, y + POINT_RADIUS,
                                          fill=color, outline="", tags=IMAGE_POINT_TAG)

    def _draw_outline_on_image(self, points: PointList) -> None:
        self.canvas_image.delete(IMAGE_OUTLINE_TAG)
        self.canvas_image.delete(IMAGE_DASH_TAG)
        if len(points) >= 2:
            self.canvas_image.create_line(
                *flatten_points(points),
                fill=COLOR_LINE,
                width=PREVIEW_LINE_WIDTH,
                tags=IMAGE_OUTLINE_TAG
            )
        if len(points) >= 3:
            last_x, last_y = points[-1]
            first_x, first_y = points[0]
            self.canvas_image.create_line(
                last_x, last_y, first_x, first_y,
                fill=COLOR_LINE,
                width=PREVIEW_LINE_WIDTH,
                dash=(4, 3),
                tags=IMAGE_DASH_TAG
            )

    def show_warning(self, title: str, message: str) -> None:
        messagebox.showwarning(title, message)

    def show_info(self, title: str, message: str) -> None:
        messagebox.showinfo(title, message)

    def show_error(self, title: str, message: str) -> None:
        messagebox.showerror(title, message)

    @property
    def suppress_listbox_event(self) -> bool:
        return self._suppress_listbox_event


class PolygonController:
    """Handle user input and keep the view in sync with the model."""

    def __init__(self, master: tk.Tk, config: AppConfig) -> None:
        self.master = master
        self.config = config
        self.model = PolygonModel(config)
        self.view = PolygonView(master)
        self.selected_index: Optional[int] = None

        self._bind_events()
        self.master.protocol("WM_DELETE_WINDOW", self.on_close)

        if self.config.mode == "edit":
            self.view.set_title("Polygon Editor - Edit Mode")
            self.view.load_button.pack_forget()
            self._load_existing()
        else:
            self.view.set_title("Polygon Selector - New Mode")

    def _bind_events(self) -> None:
        self.view.canvas_image.bind("<Button-1>", self.on_canvas_click)
        self.view.coords_listbox.bind("<<ListboxSelect>>", self.on_listbox_select)
        self.view.undo_button.config(command=self.undo_point)
        self.view.delete_button.config(command=self.delete_point)
        self.view.save_button.config(command=self.save_result)
        self.view.load_button.config(command=self.load_image)

    def _load_existing(self) -> None:
        try:
            self.model.load_existing()
            if self.model.image_handler.display_image is not None:
                self.view.set_image(self.model.image_handler)
            if self.model.points:
                self.selected_index = len(self.model.points) - 1
            self.refresh_view()
        except (OSError, json.JSONDecodeError, ValueError) as exc:
            self.view.show_error("Error", f"Failed to load existing data: {exc}")

    def refresh_view(self) -> None:
        list_points = self.model.get_scaled_points()
        display_points = self.model.get_display_points()
        self.view.update_points(list_points,
                                display_points,
                                self.selected_index)

    def on_close(self) -> None:
        if self.config.mode == "edit":
            if not self.model.saved:
                self.model.cleanup_edit_temp_files()
            self.master.destroy()
            return

        if self.model.points and not self.model.saved:
            if messagebox.askyesno("Confirm", "Exit without saving changes?"):
                self.master.destroy()
        else:
            self.master.destroy()

    def on_canvas_click(self, event: tk.Event) -> None:
        if not self.model.image_handler.has_image():
            self.view.show_warning("Warning", "No image has been loaded.")
            return

        width, height = self.model.image_handler.get_display_size()
        if event.x < 0 or event.x >= width or event.y < 0 or event.y >= height:
            self.view.show_warning("Warning", "Click is outside the image. Please click inside the image.")
            return

        self.model.add_point((event.x, event.y))
        self.selected_index = len(self.model.points) - 1
        self.refresh_view()

    def on_listbox_select(self, event: tk.Event) -> None:
        if self.view.suppress_listbox_event:
            return
        selection = self.view.coords_listbox.curselection()
        if selection:
            self.selected_index = selection[0]
        else:
            self.selected_index = None
        self.refresh_view()

    def undo_point(self) -> None:
        if self.model.undo_last_change():
            self.selected_index = len(self.model.points) - 1 if self.model.points else None
            self.refresh_view()

    def delete_point(self) -> None:
        if self.selected_index is None:
            return
        self.model.remove_point_at(self.selected_index)
        self.selected_index = None
        self.refresh_view()

    def load_image(self) -> None:
        filepath = filedialog.askopenfilename(
            filetypes=[("Image files", "*.jpg *.jpeg *.png *.bmp *.gif"),
                       ("All files", "*.*")],
            initialdir=PROJECT_ROOT_DIR
        )
        if not filepath:
            return

        try:
            self.model.load_image(filepath)
            self.view.set_image(self.model.image_handler)
            self.selected_index = None
            self.refresh_view()
        except FileNotFoundError:
            self.view.show_error("Error", "Image file not found.")
        except UnidentifiedImageError:
            self.view.show_error("Error", "Unsupported image format.")
        except OSError as exc:
            self.view.show_error("Error", f"Failed to load image: {exc}")

    def save_result(self) -> None:
        try:
            self.model.save()
        except ValueError as exc:
            self.view.show_warning("Warning", str(exc))
            return
        except OSError as exc:
            self.view.show_error("Error", f"Error while saving: {exc}")
            return

        self.view.show_info("Success", "Polygon saved successfully.")
        self.master.after(500, self.master.destroy)
