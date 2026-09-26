#include "estimators.hpp"
#include "config.hpp"
#include <cmath>
#include <algorithm>

namespace estimators {

    // --- Distance Estimator ---

    DistanceEstimator::DistanceEstimator(int frame_height) : frame_height(frame_height) {}

    double DistanceEstimator::get_real_width(int class_id) const {
        auto it = config::REAL_WIDTHS.find(class_id);
        if (it != config::REAL_WIDTHS.end()) return it->second;
        return config::DEFAULT_REAL_WIDTH;
    }

    std::map<int, double> DistanceEstimator::estimate_all(const std::vector<TrackMock>& tracks) {
        std::map<int, double> current_distances;
        
        for (const auto& t : tracks) {
            double real_width = get_real_width(t.class_id);
            double pixel_width = t.bbox[2] - t.bbox[0];
            double base_y = t.bbox[3];

            if (pixel_width < 1.0) continue;

            double geom_distance = (config::FOCAL_LENGTH * real_width) / pixel_width;
            
            double cy = base_y - (frame_height / 2.0);
            double theta = (cy / frame_height) * (M_PI / 4.0); 
            theta += config::CAMERA_PITCH_DEG * (M_PI / 180.0);
            
            double perspective_dist = config::CAMERA_HEIGHT_M / std::max(std::tan(theta), 0.01);
            double raw_dist = (1.0 - config::PERSPECTIVE_ALPHA) * geom_distance + config::PERSPECTIVE_ALPHA * perspective_dist;

            if (history.find(t.track_id) != history.end()) {
                double smoothed = config::DISTANCE_EMA_ALPHA * raw_dist + (1.0 - config::DISTANCE_EMA_ALPHA) * history[t.track_id];
                current_distances[t.track_id] = smoothed;
                history[t.track_id] = smoothed;
            } else {
                current_distances[t.track_id] = raw_dist;
                history[t.track_id] = raw_dist;
            }
        }
        
        return current_distances;
    }

    // --- Speed Estimator ---

    SpeedEstimator::SpeedEstimator(double fps) : fps(fps), dt(1.0 / fps) {}

    std::map<int, double> SpeedEstimator::update(const std::vector<TrackMock>& tracks, const std::map<int, double>& distances) {
        std::map<int, double> current_speeds;

        for (const auto& t : tracks) {
            if (distances.find(t.track_id) == distances.end()) continue;
            
            double current_dist = distances.at(t.track_id);
            dist_history[t.track_id].push_back(current_dist);

            if (dist_history[t.track_id].size() > config::SPEED_HISTORY_LEN) {
                dist_history[t.track_id].erase(dist_history[t.track_id].begin());
            }

            if (dist_history[t.track_id].size() < config::MULTI_FRAME_WINDOW) {
                if (speed_history.find(t.track_id) != speed_history.end()) {
                    current_speeds[t.track_id] = speed_history[t.track_id];
                }
                continue;
            }

            int n = dist_history[t.track_id].size();
            double d_old = dist_history[t.track_id][n - config::MULTI_FRAME_WINDOW];
            double d_new = dist_history[t.track_id][n - 1];
            
            double delta_dist = d_new - d_old;
            double time_elapsed = (config::MULTI_FRAME_WINDOW - 1) * dt;
            
            double rel_speed_ms = delta_dist / time_elapsed;
            double rel_speed_kmh = rel_speed_ms * 3.6;

            double raw_abs_speed = std::max(config::EGO_SPEED_DEFAULT + rel_speed_kmh, 0.0);

            if (speed_history.find(t.track_id) != speed_history.end()) {
                double last_speed = speed_history[t.track_id];
                if (std::abs(raw_abs_speed - last_speed) > config::SPEED_MAX_JUMP_KMH) {
                    raw_abs_speed = last_speed; 
                }
                double smoothed = config::SPEED_EMA_ALPHA * raw_abs_speed + (1.0 - config::SPEED_EMA_ALPHA) * last_speed;
                current_speeds[t.track_id] = smoothed;
                speed_history[t.track_id] = smoothed;
            } else {
                current_speeds[t.track_id] = raw_abs_speed;
                speed_history[t.track_id] = raw_abs_speed;
            }
        }
        return current_speeds;
    }

    // --- FCW Engine ---

    FCWEngine::FCWEngine(double ego_speed_kmh) : ego_speed(ego_speed_kmh) {}

    std::map<int, FCWResult> FCWEngine::evaluate(const std::vector<TrackMock>& tracks, 
                                                 const std::map<int, double>& distances,
                                                 const std::map<int, double>& speeds,
                                                 double ego_speed_kmh) {
        std::map<int, FCWResult> results;
        double current_ego_speed = (ego_speed_kmh >= 0.0) ? ego_speed_kmh : ego_speed;
        double current_ego_ms = current_ego_speed / 3.6;

        for (const auto& t : tracks) {
            if (distances.find(t.track_id) == distances.end() || speeds.find(t.track_id) == speeds.end()) {
                continue;
            }

            double dist = distances.at(t.track_id);
            double speed = speeds.at(t.track_id);
            double target_ms = speed / 3.6;

            double rel_speed_ms = current_ego_ms - target_ms;

            double ttc = 999.0;
            if (rel_speed_ms > 0.5) {
                ttc = dist / rel_speed_ms;
            }

            double req_decel = 0.0;
            if (rel_speed_ms > 0 && dist > 0) {
                req_decel = (rel_speed_ms * rel_speed_ms) / (2.0 * dist);
            }

            std::string raw_alert = "SAFE";
            if ((ttc < config::TTC_BRAKE || req_decel > 4.0) && dist < config::DIST_BRAKE * 2) {
                raw_alert = "BRAKE";
            } else if ((ttc < config::TTC_CAUTION || req_decel > 2.0) && dist < config::DIST_CAUTION * 2) {
                raw_alert = "CAUTION";
            }

            if (alert_counters.find(t.track_id) == alert_counters.end()) {
                alert_counters[t.track_id] = 0;
            }

            if (raw_alert == "BRAKE") alert_counters[t.track_id] += 2;
            else if (raw_alert == "CAUTION") alert_counters[t.track_id] += 1;
            else alert_counters[t.track_id] = std::max(0, alert_counters[t.track_id] - 1);

            std::string final_alert = "SAFE";
            if (alert_counters[t.track_id] >= config::FCW_HYSTERESIS_FRAMES * 2) final_alert = "BRAKE";
            else if (alert_counters[t.track_id] >= config::FCW_HYSTERESIS_FRAMES) final_alert = "CAUTION";

            FCWResult res = {final_alert, ttc, dist, speed, req_decel};
            results[t.track_id] = res;
        }

        return results;
    }

}
