"""
config.py
---------
Centralised configuration for the ADAS perception pipeline.

All tuneable parameters live here. Override via environment variables
or a `.env` file in the project root.

Usage:
    from config import cfg
    print(cfg.FOCAL_LENGTH)
"""

import os
import logging
from dataclasses import dataclass, field
from typing import List

logger = logging.getLogger("adas")


# ---------------------------------------------------------------------------
# Device auto-detection
# ---------------------------------------------------------------------------

def _auto_device() -> str:
    """Detect best available inference device: cuda → mps → cpu."""
    try:
        import torch
        if torch.cuda.is_available():
            return "cuda"
        if hasattr(torch.backends, "mps") and torch.backends.mps.is_available():
            return "mps"
    except ImportError:
        pass
    return "cpu"


# ---------------------------------------------------------------------------
# Configuration dataclass
# ---------------------------------------------------------------------------

@dataclass
class ADASConfig:
    """All tuneable parameters for the ADAS pipeline."""

    # ── General ──────────────────────────────────────────────────────────
    DEVICE: str = field(default_factory=_auto_device)
    LOG_LEVEL: str = "INFO"
    MAX_UPLOAD_MB: int = 500

    # ── Preprocessing ────────────────────────────────────────────────────
    TARGET_WIDTH: int = 1280
    TARGET_HEIGHT: int = 720
    CLAHE_CLIP_LIMIT: float = 2.0
    CLAHE_TILE_SIZE: int = 8
    GAUSSIAN_KERNEL: int = 5
    USE_ADAPTIVE_CANNY: bool = True

    # ── ROI ──────────────────────────────────────────────────────────────
    ROI_BOTTOM_LEFT_X: float = 0.05
    ROI_TOP_LEFT_X: float = 0.40
    ROI_TOP_RIGHT_X: float = 0.60
    ROI_BOTTOM_RIGHT_X: float = 0.95
    ROI_TOP_Y: float = 0.58
    ROI_BOTTOM_Y: float = 0.85

    # ── Lane Detection ───────────────────────────────────────────────────
    HOUGH_RHO: int = 1
    HOUGH_THRESHOLD: int = 30
    HOUGH_MIN_LEN: int = 40
    HOUGH_MAX_GAP: int = 100
    SLOPE_MIN: float = 0.4
    SLOPE_MAX: float = 10.0
    LANE_SMOOTH_WINDOW: int = 10
    LANE_EMA_ALPHA: float = 0.3
    LANE_MIN_SEGMENTS: int = 2  # minimum segments to accept a lane

    # ── Vehicle Detection ────────────────────────────────────────────────
    YOLO_CONF: float = 0.4
    YOLO_IOU: float = 0.45
    VEHICLE_CLASSES: dict = field(default_factory=lambda: {
        0: "person",
        1: "bicycle",
        2: "car",
        3: "motorcycle",
        5: "bus",
        7: "truck",
    })
    DETECTION_ROI_TOP: float = 0.30  # ignore detections above this line
    MIN_DETECTION_AREA: int = 500     # minimum bbox area in pixels

    # ── Tracker ──────────────────────────────────────────────────────────
    IOU_THRESHOLD: float = 0.25
    MAX_AGE: int = 8
    MIN_HITS: int = 2
    BBOX_SMOOTH_ALPHA: float = 0.5
    USE_KALMAN: bool = True

    # ── Distance Estimation ──────────────────────────────────────────────
    FOCAL_LENGTH: float = 850.0
    CAMERA_HEIGHT_M: float = 1.3        # typical dashcam mount height
    CAMERA_PITCH_DEG: float = 2.0       # slight downward pitch
    PERSPECTIVE_ALPHA: float = 0.5
    DISTANCE_EMA_ALPHA: float = 0.4
    REAL_WIDTHS: dict = field(default_factory=lambda: {
        0: 0.5,     # person
        1: 0.6,     # bicycle
        2: 1.8,     # car
        3: 0.8,     # motorcycle
        5: 2.5,     # bus
        7: 2.4,     # truck
    })
    DEFAULT_REAL_WIDTH: float = 1.8

    # ── Speed Estimation ─────────────────────────────────────────────────
    PIXELS_PER_METRE_AT_REF: float = 153.0
    REFERENCE_DISTANCE_M: float = 10.0
    SPEED_EMA_ALPHA: float = 0.35
    SPEED_HISTORY_LEN: int = 10
    SPEED_MAX_JUMP_KMH: float = 50.0   # reject unrealistic jumps
    MULTI_FRAME_WINDOW: int = 3

    # ── FCW ──────────────────────────────────────────────────────────────
    TTC_BRAKE: float = 1.5
    TTC_CAUTION: float = 3.0
    DIST_BRAKE: float = 10.0
    DIST_CAUTION: float = 20.0
    EGO_SPEED_DEFAULT: float = 60.0
    FCW_HYSTERESIS_FRAMES: int = 3      # frames before alert level upgrades

    # ── Visualization ────────────────────────────────────────────────────
    HUD_OPACITY: float = 0.70
    TRAIL_LENGTH: int = 15
    SHOW_MINIMAP: bool = True
    MINIMAP_SIZE: int = 180

    # ── Server ───────────────────────────────────────────────────────────
    CORS_ORIGINS: list = field(default_factory=lambda: ["*"])
    API_HOST: str = "0.0.0.0"
    API_PORT: int = 8000

    def __post_init__(self):
        """Override fields from environment variables if set."""
        for fld in self.__dataclass_fields__:
            env_key = f"ADAS_{fld}"
            env_val = os.environ.get(env_key)
            if env_val is not None:
                field_type = type(getattr(self, fld))
                try:
                    if field_type == bool:
                        setattr(self, fld, env_val.lower() in ("1", "true", "yes"))
                    elif field_type == dict or field_type == list:
                        pass  # skip complex types for env override
                    else:
                        setattr(self, fld, field_type(env_val))
                except (ValueError, TypeError):
                    logger.warning(f"Could not parse env var {env_key}={env_val}")


# ---------------------------------------------------------------------------
# Global singleton
# ---------------------------------------------------------------------------

# Load .env file if python-dotenv is available
try:
    from dotenv import load_dotenv
    load_dotenv()
except ImportError:
    pass

cfg = ADASConfig()
