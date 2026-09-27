/**
 * @file    config.hpp
 * @brief   Centralised configuration constants for the ADAS perception pipeline.
 *
 * All tuneable parameters are declared as compile-time constants in the
 * `config` namespace. Modifying a value here propagates to every module
 * that includes this header, ensuring a single source of truth.
 *
 * Parameter groups:
 *   - Preprocessing (resolution, filtering)
 *   - Region of Interest (trapezoidal mask geometry)
 *   - Lane Detection (Hough Transform, temporal smoothing)
 *   - Vehicle Detection (YOLO confidence, NMS, class map)
 *   - Multi-Object Tracking (IoU, Kalman, lifecycle)
 *   - Distance Estimation (pinhole camera model)
 *   - Speed Estimation (pixel-displacement, EMA)
 *   - Forward Collision Warning (TTC thresholds, hysteresis)
 *   - Visualization (HUD, minimap, trails)
 *
 * @note    Focal length and camera height are assumed values for a generic
 *          1280x720 dashcam. For production deployment, these must be
 *          replaced with calibrated intrinsic/extrinsic parameters.
 *
 * @author  Om Jagdale
 */

#pragma once

#include <string>
#include <map>

namespace config {

    // ─────────────────────────────────────────────────────────────────
    //  General
    // ─────────────────────────────────────────────────────────────────

    /** @brief Default inference device ("cpu", "cuda", or "mps"). */
    const std::string DEVICE = "cpu";

    /** @brief Logging verbosity: "DEBUG", "INFO", "WARNING", "ERROR". */
    const std::string LOG_LEVEL = "INFO";

    /** @brief Maximum upload file size in megabytes. */
    const int MAX_UPLOAD_MB = 500;

    // ─────────────────────────────────────────────────────────────────
    //  Preprocessing
    // ─────────────────────────────────────────────────────────────────

    /** @brief Target frame width after resize (pixels). */
    const int TARGET_WIDTH = 1280;

    /** @brief Target frame height after resize (pixels). */
    const int TARGET_HEIGHT = 720;

    /** @brief CLAHE clip limit for contrast enhancement. */
    const double CLAHE_CLIP_LIMIT = 2.0;

    /** @brief CLAHE tile grid size (NxN). */
    const int CLAHE_TILE_SIZE = 8;

    /** @brief Gaussian blur kernel size (must be odd). */
    const int GAUSSIAN_KERNEL = 5;

    /** @brief Use Otsu-based adaptive Canny thresholds. */
    const bool USE_ADAPTIVE_CANNY = true;

    // ─────────────────────────────────────────────────────────────────
    //  Region of Interest (trapezoidal mask)
    // ─────────────────────────────────────────────────────────────────

    /** @brief ROI vertex positions as fractions of frame dimensions. */
    const double ROI_BOTTOM_LEFT_X = 0.05;
    const double ROI_TOP_LEFT_X = 0.40;
    const double ROI_TOP_RIGHT_X = 0.60;
    const double ROI_BOTTOM_RIGHT_X = 0.95;
    const double ROI_TOP_Y = 0.58;
    const double ROI_BOTTOM_Y = 0.85;

    // ─────────────────────────────────────────────────────────────────
    //  Lane Detection (Probabilistic Hough Transform)
    // ─────────────────────────────────────────────────────────────────

    /** @brief Hough accumulator resolution in pixels. */
    const int HOUGH_RHO = 1;

    /** @brief Minimum number of votes to accept a line. */
    const int HOUGH_THRESHOLD = 30;

    /** @brief Minimum segment length in pixels. */
    const int HOUGH_MIN_LEN = 40;

    /** @brief Maximum gap between collinear segments to merge. */
    const int HOUGH_MAX_GAP = 100;

    /** @brief Minimum absolute slope to accept a lane candidate. */
    const double SLOPE_MIN = 0.4;

    /** @brief Maximum absolute slope to accept a lane candidate. */
    const double SLOPE_MAX = 10.0;

    /** @brief Temporal smoothing window length (frames). */
    const int LANE_SMOOTH_WINDOW = 10;

    /** @brief EMA blending factor for lane parameter smoothing. */
    const double LANE_EMA_ALPHA = 0.3;

    /** @brief Minimum line segments required to form a lane. */
    const int LANE_MIN_SEGMENTS = 2;

    // ─────────────────────────────────────────────────────────────────
    //  Vehicle Detection (YOLOv11n)
    // ─────────────────────────────────────────────────────────────────

    /** @brief YOLO confidence threshold for accepting detections. */
    const double YOLO_CONF = 0.4;

    /** @brief Non-Maximum Suppression IoU threshold. */
    const double YOLO_IOU = 0.45;

    /** @brief Upper boundary of detection ROI (fraction of frame height). */
    const double DETECTION_ROI_TOP = 0.30;

    /** @brief Minimum bounding box area (pixels^2) to accept. */
    const int MIN_DETECTION_AREA = 500;

    /**
     * @brief COCO class ID to human-readable label mapping.
     * Only vehicle-relevant classes are included for filtering.
     */
    const std::map<int, std::string> VEHICLE_CLASSES = {
        {0, "person"},
        {1, "bicycle"},
        {2, "car"},
        {3, "motorcycle"},
        {5, "bus"},
        {7, "truck"}
    };

    // ─────────────────────────────────────────────────────────────────
    //  Multi-Object Tracking (IoU + Kalman)
    // ─────────────────────────────────────────────────────────────────

    /** @brief Minimum IoU overlap to associate a detection with a track. */
    const double IOU_THRESHOLD = 0.25;

    /** @brief Maximum frames a track survives without a matching detection. */
    const int MAX_AGE = 8;

    /** @brief Consecutive matches required before a track is confirmed. */
    const int MIN_HITS = 2;

    /** @brief EMA factor for bounding box coordinate smoothing. */
    const double BBOX_SMOOTH_ALPHA = 0.5;

    /** @brief Enable Kalman filter for track state prediction. */
    const bool USE_KALMAN = true;

    // ─────────────────────────────────────────────────────────────────
    //  Distance Estimation (Pinhole Camera Model)
    // ─────────────────────────────────────────────────────────────────

    /** @brief Assumed focal length in pixels (for 1280x720 resolution). */
    const double FOCAL_LENGTH = 850.0;

    /** @brief Assumed camera mounting height above ground (metres). */
    const double CAMERA_HEIGHT_M = 1.3;

    /** @brief Camera pitch angle relative to horizontal (degrees). */
    const double CAMERA_PITCH_DEG = 2.0;

    /** @brief Perspective correction scaling factor. */
    const double PERSPECTIVE_ALPHA = 0.5;

    /** @brief EMA factor for distance estimate smoothing. */
    const double DISTANCE_EMA_ALPHA = 0.4;

    /**
     * @brief Known real-world widths of vehicle classes (metres).
     * Used in the pinhole camera distance formula.
     */
    const std::map<int, double> REAL_WIDTHS = {
        {0, 0.5}, {1, 0.6}, {2, 1.8}, {3, 0.8}, {5, 2.5}, {7, 2.4}
    };

    /** @brief Default vehicle width when class is unknown. */
    const double DEFAULT_REAL_WIDTH = 1.8;

    // ─────────────────────────────────────────────────────────────────
    //  Speed Estimation (Pixel Displacement + EMA)
    // ─────────────────────────────────────────────────────────────────

    /** @brief Reference pixel-per-metre ratio at REFERENCE_DISTANCE_M. */
    const double PIXELS_PER_METRE_AT_REF = 153.0;

    /** @brief Reference distance for the pixel scale calibration. */
    const double REFERENCE_DISTANCE_M = 10.0;

    /** @brief EMA factor for speed smoothing. */
    const double SPEED_EMA_ALPHA = 0.35;

    /** @brief Number of past speed samples to retain per track. */
    const int SPEED_HISTORY_LEN = 10;

    /** @brief Maximum plausible speed change between frames (km/h). */
    const double SPEED_MAX_JUMP_KMH = 50.0;

    /** @brief Number of frames used for multi-frame speed averaging. */
    const int MULTI_FRAME_WINDOW = 3;

    // ─────────────────────────────────────────────────────────────────
    //  Forward Collision Warning (FCW)
    // ─────────────────────────────────────────────────────────────────

    /** @brief TTC threshold for BRAKE alert (seconds). */
    const double TTC_BRAKE = 1.5;

    /** @brief TTC threshold for CAUTION alert (seconds). */
    const double TTC_CAUTION = 3.0;

    /** @brief Distance threshold for BRAKE alert (metres). */
    const double DIST_BRAKE = 10.0;

    /** @brief Distance threshold for CAUTION alert (metres). */
    const double DIST_CAUTION = 20.0;

    /** @brief Default ego vehicle speed when not provided (km/h). */
    const double EGO_SPEED_DEFAULT = 60.0;

    /** @brief Frames an alert must persist before promotion (hysteresis). */
    const int FCW_HYSTERESIS_FRAMES = 3;

    // ─────────────────────────────────────────────────────────────────
    //  Visualization
    // ─────────────────────────────────────────────────────────────────

    /** @brief HUD background overlay opacity [0.0, 1.0]. */
    const double HUD_OPACITY = 0.70;

    /** @brief Number of past centroid positions to draw as a trail. */
    const int TRAIL_LENGTH = 15;

    /** @brief Whether to render the bird's-eye-view minimap. */
    const bool SHOW_MINIMAP = true;

    /** @brief Minimap square dimension in pixels. */
    const int MINIMAP_SIZE = 180;
}
