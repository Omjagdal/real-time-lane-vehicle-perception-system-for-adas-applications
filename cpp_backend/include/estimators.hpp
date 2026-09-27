/**
 * @file    estimators.hpp
 * @brief   Distance, speed, and forward collision warning estimation modules.
 *
 * Contains three tightly coupled estimators that form the kinematic
 * estimation layer of the ADAS pipeline:
 *
 *   - **DistanceEstimator**: Monocular distance using the pinhole camera model
 *     with perspective correction and temporal EMA smoothing.
 *   - **SpeedEstimator**: Per-vehicle speed derived from frame-to-frame
 *     distance changes with multi-frame averaging and outlier rejection.
 *   - **FCWEngine**: Time-To-Collision (TTC) computation and three-tier
 *     alert classification with hysteresis-based suppression.
 *
 * @note    Distance and speed estimates are approximate (monocular, uncalibrated).
 *          For production use, these should be replaced or fused with
 *          radar/LiDAR measurements.
 *
 * @see     config.hpp for all estimation parameters and thresholds.
 * @author  Om Jagdale
 */

#pragma once

#include <vector>
#include <map>
#include <string>

namespace estimators {

    /**
     * @brief Lightweight track representation for the estimation layer.
     *
     * Contains only the fields required by distance/speed/FCW computation,
     * decoupling the estimators from the full tracker::Track struct.
     */
    struct TrackMock {
        int track_id;               ///< Persistent track identifier.
        std::vector<int> bbox;      ///< Bounding box as [x, y, width, height].
        int class_id;               ///< COCO class ID for real-width lookup.
        int age;                    ///< Track age in frames (for smoothing ramp-up).
    };

    /**
     * @brief Forward Collision Warning evaluation result for a single vehicle.
     */
    struct FCWResult {
        std::string alert;          ///< Alert level: "SAFE", "CAUTION", or "BRAKE".
        double ttc;                 ///< Time-To-Collision in seconds (INF if not closing).
        double current_dist;        ///< Current estimated distance in metres.
        double current_speed;       ///< Current estimated speed in km/h.
        double req_decel;           ///< Required deceleration to avoid collision (m/s^2).
    };

    /**
     * @brief Monocular distance estimator using the pinhole camera model.
     *
     * Formula: distance = (real_width * focal_length) / pixel_width
     *
     * Applies perspective correction based on the bounding box vertical
     * position and EMA temporal smoothing per track.
     */
    class DistanceEstimator {
    public:
        /**
         * @brief Construct with the frame height (needed for perspective correction).
         * @param frame_height  Target frame height in pixels.
         */
        DistanceEstimator(int frame_height);

        /**
         * @brief Estimate distance for all tracked vehicles.
         * @param tracks    Current frame's confirmed tracks.
         * @return          Map of track_id → distance (metres), clamped to [1, 200].
         */
        std::map<int, double> estimate_all(const std::vector<TrackMock>& tracks);

    private:
        int frame_height;                       ///< Frame height for perspective correction.
        std::map<int, double> history;          ///< Per-track EMA distance history.

        /**
         * @brief Look up the assumed real-world width for a COCO class ID.
         * @param class_id  COCO class identifier.
         * @return          Real width in metres (defaults to 1.8m if unknown).
         */
        double get_real_width(int class_id) const;
    };

    /**
     * @brief Per-vehicle speed estimator from frame-to-frame distance changes.
     *
     * Maintains a sliding window of distance measurements per track and
     * derives speed using multi-frame displacement divided by elapsed time.
     * Applies EMA smoothing and rejects implausible speed jumps.
     */
    class SpeedEstimator {
    public:
        /**
         * @brief Construct with the source video frame rate.
         * @param fps   Frames per second of the input video.
         */
        SpeedEstimator(double fps);

        /**
         * @brief Update speed estimates for all tracked vehicles.
         * @param tracks        Current frame's confirmed tracks.
         * @param distances     Per-track distance estimates from DistanceEstimator.
         * @return              Map of track_id → speed (km/h), clamped to [0, 250].
         */
        std::map<int, double> update(const std::vector<TrackMock>& tracks, 
                                     const std::map<int, double>& distances);

    private:
        double fps;                                             ///< Source video FPS.
        double dt;                                              ///< Inter-frame time interval (1/fps).
        std::map<int, std::vector<double>> dist_history;        ///< Per-track distance sliding window.
        std::map<int, double> speed_history;                    ///< Per-track EMA speed history.
    };

    /**
     * @brief Forward Collision Warning engine with TTC-based three-tier alerts.
     *
     * Evaluates each tracked vehicle against the ego vehicle's speed to
     * compute Time-To-Collision (TTC) and classify the threat level.
     * Implements hysteresis-based alert suppression to prevent rapid
     * flickering between alert states.
     *
     * Alert levels (aligned with ISO 15622 principles):
     *   - **SAFE**:    TTC >= 3.0s AND distance >= 20m
     *   - **CAUTION**: TTC <  3.0s OR  distance <  20m
     *   - **BRAKE**:   TTC <  1.5s OR  distance <  10m
     */
    class FCWEngine {
    public:
        /**
         * @brief Construct with the ego vehicle's speed.
         * @param ego_speed_kmh     Ego vehicle speed in km/h.
         */
        FCWEngine(double ego_speed_kmh);

        /**
         * @brief Evaluate FCW status for all tracked vehicles.
         * @param tracks            Current confirmed tracks.
         * @param distances         Per-track distances (metres).
         * @param speeds            Per-track speeds (km/h).
         * @param ego_speed_kmh     Override ego speed (-1.0 to use constructor value).
         * @return                  Map of track_id → FCWResult.
         */
        std::map<int, FCWResult> evaluate(const std::vector<TrackMock>& tracks, 
                                          const std::map<int, double>& distances,
                                          const std::map<int, double>& speeds,
                                          double ego_speed_kmh = -1.0);

    private:
        double ego_speed;                       ///< Ego vehicle speed in m/s.
        std::map<int, int> alert_counters;      ///< Per-track hysteresis counters.
    };

}
