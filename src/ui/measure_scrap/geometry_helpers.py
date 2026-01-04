from typing import List, Tuple


PointList = List[Tuple[float, float]]


def centroid(points: PointList) -> Tuple[float, float]:
    """Return centroid (mean) of points."""
    if not points:
        raise ValueError("No points provided")
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    return (sum(xs) / len(xs), sum(ys) / len(ys))


def scale_points(points: PointList,
                 scale: float,
                 origin: Tuple[float, float]) -> PointList:
    """Scale points around origin and return a new list."""
    ox, oy = origin
    return [((x - ox) * scale, (y - oy) * scale) for x, y in points]


def flatten_points(points: PointList) -> List[float]:
    """Flatten [(x, y), ...] into [x, y, x, y, ...]."""
    return [coord for point in points for coord in point]
