/**
 * @file    estimators.cpp
 * @brief   Distance, speed, and FCW estimation implementations.
 *
 * Implements the kinematic estimation layer:
 *
 *   - **DistanceEstimator**: Fuses geometric (pinhole) and perspective
 *     (ground-plane projection) distance models, weighted by
 *     config::PERSPECTIVE_ALPHA.
 *
 *   - **SpeedEstimator**: Derives per-vehicle speed from multi-frame
 *     distance changes. Uses a sliding window to reject measurement
 *     noise and an EMA filter for temporal smoothing.
 *
 *   - **FCWEngine**: Computes Time-To-Collision (TTC) and required
 *     deceleration for each tracked vehicle. Applies hysteresis-based
 *     alert promotion/demotion to prevent alert flickering.
 *
 * @note    All estimates are monocular approximations. For production ADAS,
 *          fuse with radar or stereo depth for metric accuracy.
 *
 * @author  Om Jagdale
 */

#include "estimators.hpp"
#include "config.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

namespace estimators {

    // ─────────────────────────────────────────────────────────────────
    //  Distance Estimator (Pinhole + Perspective Fusion)
    // ─────────────────────────────────────────────────────────────────

    DistanceEstimator::DistanceEstimator(int frame_height)
        : frame_height(frame_height) {}

    double DistanceEstimator::get_real_width(int class_id) const {
        auto it = config::REAL_WIDTHS.find(class_id);
        return (it != config::REAL_WIDTHS.end()) ? it->second : config::DEFAULT_REAL_WIDTH;
    }

    std::map<int, double> DistanceEstimator::estimate_all(
        const std::vector<TrackMock>& tracks
    ) {
        std::map<int, double> current;

        for (const auto& t : tracks) {
            const double real_width  = get_real_width(t.class_id);
            const double pixel_width = static_cast<double>(t.bbox[2] - t.bbox[0]);
            const double base_y      = static_cast<double>(t.bbox[3]);

            if (pixel_width < 1.0) continue;

            // Geometric distance (pinhole camera model)
            const double geom_dist = (config::FOCAL_LENGTH * real_width) / pixel_width;
            
            // Perspective distance (ground-plane projection)
            const double cy    = base_y - (frame_height / 2.0);
            double theta       = (cy / frame_height) * (M_PI / 4.0); 
            theta             += config::CAMERA_PITCH_DEG * (M_PI / 180.0);
            const double persp = config::CAMERA_HEIGHT_M / std::max(std::tan(theta), 0.01);

            // Weighted fusion
            const double raw = (1.0 - config::PERSPECTIVE_ALPHA) * geom_dist
                             + config::PERSPECTIVE_ALPHA * persp;

            // EMA temporal smoothing
            double smoothed;
            if (auto it = history.find(t.track_id); it != history.end()) {
                smoothed = config::DISTANCE_EMA_ALPHA * raw
                         + (1.0 - config::DISTANCE_EMA_ALPHA) * it->second;
            } else {
                smoothed = raw;
            }

            // Clamp to physical range [1, 200] metres
            smoothed = std::clamp(smoothed, 1.0, 200.0);

            current[t.track_id] = smoothed;
            history[t.track_id] = smoothed;
        }
        
        return current;
    }

    // ─────────────────────────────────────────────────────────────────
    //  Speed Estimator (Multi-Frame Displacement + EMA)
    // ─────────────────────────────────────────────────────────────────

    SpeedEstimator::SpeedEstimator(double fps) : fps(fps), dt(1.0 / fps) {}

    std::map<int, double> SpeedEstimator::update(
        const std::vector<TrackMock>& tracks,
        const std::map<int, double>& distances
    ) {
        std::map<int, double> current;

        for (const auto& t : tracks) {
            auto dist_it = distances.find(t.track_id);
            if (dist_it == distances.end()) continue;
            
            const double current_dist = dist_it->second;

            // Append to sliding window
            auto& hist = dist_history[t.track_id];
            hist.push_back(current_dist);
            if (static_cast<int>(hist.size()) > config::SPEED_HISTORY_LEN) {
                hist.erase(hist.begin());
            }

            // Need minimum window for multi-frame estimation
            if (static_cast<int>(hist.size()) < config::MULTI_FRAME_WINDOW) {
                if (auto it = speed_history.find(t.track_id); it != speed_history.end()) {
                    current[t.track_id] = it->second;
                }
                continue;
            }

            // Multi-frame speed: delta_distance / delta_time
            const int n = static_cast<int>(hist.size());
            const double d_old = hist[static_cast<size_t>(n - config::MULTI_FRAME_WINDOW)];
            const double d_new = hist[static_cast<size_t>(n - 1)];
            
            const double delta_dist    = d_new - d_old;
            const double time_elapsed  = (config::MULTI_FRAME_WINDOW - 1) * dt;
            const double rel_speed_ms  = delta_dist / time_elapsed;
            const double rel_speed_kmh = rel_speed_ms * 3.6;

            double raw_speed = std::max(config::EGO_SPEED_DEFAULT + rel_speed_kmh, 0.0);

            // Outlier rejection: reject implausible speed jumps
            if (auto it = speed_history.find(t.track_id); it != speed_history.end()) {
                const double last_speed = it->second;
                if (std::abs(raw_speed - last_speed) > config::SPEED_MAX_JUMP_KMH) {
                    raw_speed = last_speed;
                }
                // EMA smoothing
                const double smoothed = config::SPEED_EMA_ALPHA * raw_speed
                                      + (1.0 - config::SPEED_EMA_ALPHA) * last_speed;
                current[t.track_id]       = std::clamp(smoothed, 0.0, 250.0);
                speed_history[t.track_id] = current[t.track_id];
            } else {
                current[t.track_id]       = std::clamp(raw_speed, 0.0, 250.0);
                speed_history[t.track_id] = current[t.track_id];
            }
        }

        return current;
    }

    // ─────────────────────────────────────────────────────────────────
    //  FCW Engine (TTC + Required Deceleration + Hysteresis)
    // ─────────────────────────────────────────────────────────────────

    FCWEngine::FCWEngine(double ego_speed_kmh) : ego_speed(ego_speed_kmh) {}

    std::map<int, FCWResult> FCWEngine::evaluate(
        const std::vector<TrackMock>& tracks, 
        const std::map<int, double>& distances,
        const std::map<int, double>& speeds,
        double ego_speed_kmh
    ) {
        std::map<int, FCWResult> results;
        const double current_ego = (ego_speed_kmh >= 0.0) ? ego_speed_kmh : ego_speed;
        const double ego_ms = current_ego / 3.6;

        for (const auto& t : tracks) {
            auto dist_it  = distances.find(t.track_id);
            auto speed_it = speeds.find(t.track_id);
            if (dist_it == distances.end() || speed_it == speeds.end()) continue;

            const double dist       = dist_it->second;
            const double target_kmh = speed_it->second;
            const double target_ms  = target_kmh / 3.6;
            const double rel_ms     = ego_ms - target_ms;  // Closing speed

            // Time-To-Collision (only meaningful when closing)
            const double ttc = (rel_ms > 0.5) ? (dist / rel_ms) : 999.0;

            // Required deceleration to stop before impact
            //   v^2 = u^2 - 2as  →  a = v^2 / (2s)
            const double req_decel = (rel_ms > 0.0 && dist > 0.0)
                ? (rel_ms * rel_ms) / (2.0 * dist)
                : 0.0;

            // ── Raw alert classification ─────────────────────────────
            std::string raw_alert = "SAFE";
            if ((ttc < config::TTC_BRAKE || req_decel > 4.0) && dist < config::DIST_BRAKE * 2) {
                raw_alert = "BRAKE";
            } else if ((ttc < config::TTC_CAUTION || req_decel > 2.0) && dist < config::DIST_CAUTION * 2) {
                raw_alert = "CAUTION";
            }

            // ── Hysteresis: prevent alert flickering ─────────────────
            auto& counter = alert_counters[t.track_id];
            if (raw_alert == "BRAKE") {
                counter += 2;
            } else if (raw_alert == "CAUTION") {
                counter += 1;
            } else {
                counter = std::max(0, counter - 1);
            }

            std::string final_alert = "SAFE";
            if (counter >= config::FCW_HYSTERESIS_FRAMES * 2) {
                final_alert = "BRAKE";
            } else if (counter >= config::FCW_HYSTERESIS_FRAMES) {
                final_alert = "CAUTION";
            }

            results[t.track_id] = FCWResult{
                final_alert, ttc, dist, target_kmh, req_decel
            };
        }

        return results;
    }

}
