#pragma once

#include <string>
#include <map>

namespace config {

    // General
    const std::string DEVICE = "cpu";
    const std::string LOG_LEVEL = "INFO";
    const int MAX_UPLOAD_MB = 500;

    // Preprocessing
    const int TARGET_WIDTH = 1280;
    const int TARGET_HEIGHT = 720;
    const double CLAHE_CLIP_LIMIT = 2.0;
    const int CLAHE_TILE_SIZE = 8;
    const int GAUSSIAN_KERNEL = 5;
    const bool USE_ADAPTIVE_CANNY = true;

    // ROI
    const double ROI_BOTTOM_LEFT_X = 0.05;
    const double ROI_TOP_LEFT_X = 0.40;
    const double ROI_TOP_RIGHT_X = 0.60;
    const double ROI_BOTTOM_RIGHT_X = 0.95;
    const double ROI_TOP_Y = 0.58;
    const double ROI_BOTTOM_Y = 0.85;

    // Lane Detection
    const int HOUGH_RHO = 1;
    const int HOUGH_THRESHOLD = 30;
    const int HOUGH_MIN_LEN = 40;
    const int HOUGH_MAX_GAP = 100;
    const double SLOPE_MIN = 0.4;
    const double SLOPE_MAX = 10.0;
    const int LANE_SMOOTH_WINDOW = 10;
    const double LANE_EMA_ALPHA = 0.3;
    const int LANE_MIN_SEGMENTS = 2;

    // Vehicle Detection
    const double YOLO_CONF = 0.4;
    const double YOLO_IOU = 0.45;
    const double DETECTION_ROI_TOP = 0.30;
    const int MIN_DETECTION_AREA = 500;

    const std::map<int, std::string> VEHICLE_CLASSES = {
        {0, "person"},
        {1, "bicycle"},
        {2, "car"},
        {3, "motorcycle"},
        {5, "bus"},
        {7, "truck"}
    };

    // Tracker
    const double IOU_THRESHOLD = 0.25;
    const int MAX_AGE = 8;
    const int MIN_HITS = 2;
    const double BBOX_SMOOTH_ALPHA = 0.5;
    const bool USE_KALMAN = true;

    // Distance Estimation
    const double FOCAL_LENGTH = 850.0;
    const double CAMERA_HEIGHT_M = 1.3;
    const double CAMERA_PITCH_DEG = 2.0;
    const double PERSPECTIVE_ALPHA = 0.5;
    const double DISTANCE_EMA_ALPHA = 0.4;

    const std::map<int, double> REAL_WIDTHS = {
        {0, 0.5}, {1, 0.6}, {2, 1.8}, {3, 0.8}, {5, 2.5}, {7, 2.4}
    };
    const double DEFAULT_REAL_WIDTH = 1.8;

    // Speed Estimation
    const double PIXELS_PER_METRE_AT_REF = 153.0;
    const double REFERENCE_DISTANCE_M = 10.0;
    const double SPEED_EMA_ALPHA = 0.35;
    const int SPEED_HISTORY_LEN = 10;
    const double SPEED_MAX_JUMP_KMH = 50.0;
    const int MULTI_FRAME_WINDOW = 3;

    // FCW
    const double TTC_BRAKE = 1.5;
    const double TTC_CAUTION = 3.0;
    const double DIST_BRAKE = 10.0;
    const double DIST_CAUTION = 20.0;
    const double EGO_SPEED_DEFAULT = 60.0;
    const int FCW_HYSTERESIS_FRAMES = 3;

    // Visualization
    const double HUD_OPACITY = 0.70;
    const int TRAIL_LENGTH = 15;
    const bool SHOW_MINIMAP = true;
    const int MINIMAP_SIZE = 180;
}
