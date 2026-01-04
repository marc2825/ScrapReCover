from typing import List, Optional, Tuple

from PIL import Image, ImageDraw


MASK_FILL = 255


class ImageHandler:
    """Handle image loading, resizing, and cropping."""

    def __init__(self, max_display_size: int) -> None:
        self.max_display_size = max_display_size
        self.source_image: Optional[Image.Image] = None
        self.display_image: Optional[Image.Image] = None
        self.display_scale: float = 1.0
        self.image_path: Optional[str] = None

    def load(self, path: str) -> None:
        """Load an image and create a display-sized copy."""
        self.image_path = path
        self.source_image = Image.open(path)
        self._update_display_image()

    def _update_display_image(self) -> None:
        if self.source_image is None:
            self.display_image = None
            self.display_scale = 1.0
            return

        width, height = self.source_image.size
        scale = min(1.0, self.max_display_size / width, self.max_display_size / height)
        display_width = int(width * scale)
        display_height = int(height * scale)
        self.display_scale = scale
        if scale < 1.0:
            self.display_image = self.source_image.resize((display_width, display_height), Image.LANCZOS)
        else:
            self.display_image = self.source_image.copy()

    def has_image(self) -> bool:
        return self.source_image is not None

    def get_display_size(self) -> Tuple[int, int]:
        if self.display_image is None:
            return (0, 0)
        return self.display_image.size

    def display_to_source(self, point: Tuple[float, float]) -> Tuple[float, float]:
        if not self.source_image or self.display_scale <= 0.0:
            return point
        return (point[0] / self.display_scale, point[1] / self.display_scale)

    def source_to_display(self, point: Tuple[float, float]) -> Tuple[float, float]:
        if not self.source_image:
            return point
        return (point[0] * self.display_scale, point[1] * self.display_scale)

    def make_texture(self, source_points: List[Tuple[float, float]]) -> Image.Image:
        if not self.display_image:
            raise ValueError("No display image loaded")
        mask = Image.new('L', self.display_image.size, 0)
        draw = ImageDraw.Draw(mask)
        draw.polygon(source_points, fill=MASK_FILL)

        rgba_image = self.display_image.convert("RGBA")
        rgba_image.putalpha(mask)

        bbox = mask.getbbox()
        if bbox:
            return rgba_image.crop(bbox)
        return rgba_image
