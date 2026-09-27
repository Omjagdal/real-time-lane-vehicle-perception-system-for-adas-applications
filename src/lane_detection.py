"""
lane_detection.py
-----------------
Traditional CV-based lane detection.

Pipeline:
  1. Receive a preprocessed edge image (from preprocessing.preprocess_for_lane)
  2. Apply Probabilistic Hough Line Transform
  3. Separate line segments into left / right lanes by slope
  4. Average and extrapolate each lane to a single solid line
  5. Return (left_line, right_line) as pixel endpoints
"""

import cv2
import numpy as np
from collections import deque
from typing import Optional, Tuple, List


# ---------------------------------------------------------------------------
# Type aliases
# ---------------------------------------------------------------------------
Line  = Tuple[int, int, int, int]          # (x1, y1, x2, y2)
Lines = Optional[List[Line]]


# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
HOUGH_RHO        = 1          # Distance resolution (pixels)
HOUGH_THETA      = np.pi / 180
HOUGH_THRESHOLD  = 30         # Minimum votes
HOUGH_MIN_LEN    = 40         # Minimum line length
HOUGH_MAX_GAP    = 100        # Maximum gap between segments

SLOPE_MIN        = 0.4        # Reject nearly-horizontal noise
SLOPE_MAX        = 10.0       # Reject nearly-vertical noise
ROI_TOP_RATIO    = 0.58       # Where the ROI starts (fraction from top)

SMOOTH_WINDOW    = 8          # Frames to average for temporal smoothing


# ---------------------------------------------------------------------------
# LaneDetector class
# ---------------------------------------------------------------------------

class LaneDetector:
    """Stateful lane detector with temporal smoothing."""

    def __init__(self, smooth_window: int = SMOOTH_WINDOW):
        self._left_history : deque = deque(maxlen=smooth_window)
        self._right_history: deque = deque(maxlen=smooth_window)

    # ------------------------------------------------------------------
    # Public
    # ------------------------------------------------------------------

    def detect(self,
               edge_image: np.ndarray,
               frame_height: int,
               frame_width: int) -> Tuple[Lines, Lines]:
        """
        Detect left and right lane lines.

        Parameters
        ----------
        edge_image   : Canny edge map (uint8, single-channel)
        frame_height : Height of the *original* frame
        frame_width  : Width  of the *original* frame

        Returns
        -------
        (left_line, right_line)
            Each is either None or a list with one (x1,y1,x2,y2) tuple.
        """
        raw_lines = self._hough(edge_image)
        if raw_lines is None:
            return None, None

        left_params, right_params = self._split_lines(raw_lines)

        left_line  = self._make_line(left_params,  frame_height, frame_width,
                                     self._left_history,  side="left")
        right_line = self._make_line(right_params, frame_height, frame_width,
                                     self._right_history, side="right")

        return left_line, right_line

    # ------------------------------------------------------------------
    # Private helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _hough(edge_image: np.ndarray):
        return cv2.HoughLinesP(
            edge_image,
            rho=HOUGH_RHO,
            theta=HOUGH_THETA,
            threshold=HOUGH_THRESHOLD,
            minLineLength=HOUGH_MIN_LEN,
            maxLineGap=HOUGH_MAX_GAP
        )

    @staticmethod
    def _split_lines(raw_lines) -> Tuple[List, List]:
        """
        Separate raw Hough segments into left (negative slope) and
        right (positive slope) groups.  Returns lists of (slope, intercept).
        """
        left_params  = []
        right_params = []

        for line in raw_lines:
            coords = np.ravel(line)
            if len(coords) < 4:
                continue
            x1, y1, x2, y2 = coords[:4]
            if x2 == x1:
                continue  # vertical — skip
            slope = (y2 - y1) / (x2 - x1)
            if abs(slope) < SLOPE_MIN or abs(slope) > SLOPE_MAX:
                continue
            intercept = y1 - slope * x1
            length = np.hypot(x2 - x1, y2 - y1)

            if slope < 0:
                left_params.append((slope, intercept, length))
            else:
                right_params.append((slope, intercept, length))

        return left_params, right_params

    @staticmethod
    def _weighted_average(params: List) -> Optional[Tuple[float, float]]:
        """Compute a length-weighted average of (slope, intercept) pairs."""
        if not params:
            return None
        slopes, intercepts, lengths = zip(*params)
        total = sum(lengths)
        avg_slope     = sum(s * l for s, l in zip(slopes,     lengths)) / total
        avg_intercept = sum(i * l for i, l in zip(intercepts, lengths)) / total
        return avg_slope, avg_intercept

    def _make_line(self,
                   params: List,
                   frame_height: int,
                   frame_width: int,
                   history: deque,
                   side: str) -> Lines:
        """
        Convert slope/intercept params to pixel endpoints, apply temporal
        smoothing via a rolling history, and return [(x1,y1,x2,y2)].
        """
        result = self._weighted_average(params)
        if result is not None:
            history.append(result)

        if not history:
            return None

        # Temporally smoothed slope & intercept
        avg = np.mean(history, axis=0)
        slope, intercept = avg[0], avg[1]

        if slope == 0:
            return None

        y_bottom = frame_height
        y_top    = int(frame_height * ROI_TOP_RATIO)

        x_bottom = int((y_bottom - intercept) / slope)
        x_top    = int((y_top    - intercept) / slope)

        # Clamp to frame bounds
        x_bottom = int(np.clip(x_bottom, 0, frame_width - 1))
        x_top    = int(np.clip(x_top,    0, frame_width - 1))

        return [(x_bottom, y_bottom, x_top, y_top)]

    def reset(self):
        """Clear temporal smoothing history."""
        self._left_history.clear()
        self._right_history.clear()
