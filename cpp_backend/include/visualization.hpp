/**
 * @file    visualization.hpp
 * @brief   Annotated frame rendering, HUD overlay, and bird's-eye-view minimap.
 *
 * Provides a set of composable drawing functions that overlay perception
 * results onto the video frame. Each function takes a frame by reference,
 * draws its specific overlay, and returns the modified frame.
 *
 * Rendering layers (applied in order):
 *   1. **Lane overlay**: Semi-transparent polygon fill between detected lanes.
 *   2. **Vehicle annotations**: Bounding boxes, class labels, distance, speed,
 *      colour-coded FCW alert indicators, and centroid trails.
 *   3. **FCW banner**: Full-width alert banner at the top of the frame.
 *   4. **HUD panel**: Real-time statistics overlay (FPS, vehicle count,
 *      ego speed, lane confidence, departure status).
 *   5. **Minimap**: Bird's-eye-view representation of nearby vehicles
 *      with distance and alert colour coding.
 *
 * @see     config.hpp for HUD_OPACITY, TRAIL_LENGTH, SHOW_MINIMAP, MINIMAP_SIZE.
 * @author  Om Jagdale
 */

#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <map>
#include <string>
#include <optional>
#include "tracker.hpp"
#include "estimators.hpp"
#include "lane_detection.hpp"

namespace visualization {

    /**
     * @brief Draw detected lane lines and a filled polygon between them.
     *
     * Renders the left lane in blue and the right lane in red, with a
     * semi-transparent green fill between them indicating the drivable corridor.
     * Displays the current departure status label.
     *
     * @param frame             BGR frame to annotate (modified in place).
     * @param left_line         Detected left lane segments (may be nullopt).
     * @param right_line        Detected right lane segments (may be nullopt).
     * @param departure_status  Current departure state ("CENTERED", "LEFT_DEPARTURE", etc.).
     * @return                  Reference to the annotated frame.
     */
    cv::Mat draw_lanes(cv::Mat& frame, 
                       const lane_detection::Lines& left_line, 
                       const lane_detection::Lines& right_line, 
                       const std::string& departure_status);

    /**
     * @brief Draw vehicle bounding boxes with tracking information and FCW indicators.
     *
     * For each tracked vehicle, renders:
     *   - Colour-coded bounding box (green=SAFE, yellow=CAUTION, red=BRAKE)
     *   - Track ID and class label
     *   - Estimated distance (metres) and speed (km/h)
     *   - Centroid trail showing recent trajectory
     *
     * @param frame         BGR frame to annotate.
     * @param tracks        Confirmed tracked objects.
     * @param distances     Per-track distances in metres.
     * @param speeds        Per-track speeds in km/h.
     * @param fcw_results   Per-track FCW evaluation results.
     * @return              Reference to the annotated frame.
     */
    cv::Mat draw_vehicles(cv::Mat& frame, 
                          const std::vector<tracker::Track>& tracks, 
                          const std::map<int, double>& distances, 
                          const std::map<int, double>& speeds, 
                          const std::map<int, estimators::FCWResult>& fcw_results);

    /**
     * @brief Draw a full-width FCW alert banner at the top of the frame.
     *
     * Banner colours:
     *   - "BRAKE":   Red background with white text
     *   - "CAUTION": Amber background with black text
     *   - "SAFE":    No banner rendered
     *
     * @param frame         BGR frame to annotate.
     * @param alert_level   Highest active alert level across all tracked vehicles.
     * @return              Reference to the annotated frame.
     */
    cv::Mat draw_fcw_banner(cv::Mat& frame, const std::string& alert_level);

    /**
     * @brief Draw the heads-up display (HUD) statistics panel.
     *
     * Renders a semi-transparent overlay in the top-left corner containing:
     *   - Current processing FPS
     *   - Number of active tracks
     *   - Ego vehicle speed
     *   - Left/right lane detection confidence
     *   - Lane departure status
     *
     * @param frame             BGR frame to annotate.
     * @param fps               Current processing FPS.
     * @param num_tracks        Number of confirmed tracks.
     * @param ego_speed_kmh     Ego vehicle speed in km/h.
     * @param lane_confidence   Tuple of (left, right) confidence in [0.0, 1.0].
     * @param departure_status  Current departure state string.
     * @return                  Reference to the annotated frame.
     */
    cv::Mat draw_hud(cv::Mat& frame, 
                     double fps, 
                     int num_tracks, 
                     double ego_speed_kmh, 
                     std::tuple<float, float> lane_confidence, 
                     const std::string& departure_status);

    /**
     * @brief Draw a bird's-eye-view minimap of nearby vehicles.
     *
     * Renders a top-down schematic in the bottom-right corner of the frame
     * showing the relative positions and distances of tracked vehicles.
     * Vehicles are colour-coded by their FCW alert level.
     *
     * @param frame         BGR frame to annotate.
     * @param tracks        Confirmed tracked objects.
     * @param distances     Per-track distances in metres.
     * @param fcw_results   Per-track FCW evaluation results (for colour coding).
     * @return              Reference to the annotated frame.
     */
    cv::Mat draw_minimap(cv::Mat& frame, 
                         const std::vector<tracker::Track>& tracks, 
                         const std::map<int, double>& distances, 
                         const std::map<int, estimators::FCWResult>& fcw_results);

}
