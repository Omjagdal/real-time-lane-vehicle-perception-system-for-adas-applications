/**
 * @file    tracker.cpp
 * @brief   IoU-based multi-object tracker with Kalman filter motion prediction.
 *
 * Implements the tracking layer of the ADAS pipeline. Associates incoming
 * detections with existing tracks using a greedy IoU-sorted assignment
 * strategy, with Kalman filter state prediction for motion continuity.
 *
 * Assignment strategy:
 *   1. Compute all pairwise IoU scores between predicted track positions
 *      and new detections.
 *   2. Sort candidate matches by descending IoU.
 *   3. Greedily assign the best available match for each track/detection pair.
 *   4. This approximates the Hungarian algorithm for typical ADAS scenarios
 *      where the number of tracked objects is small (< 20).
 *
 * @note    For scenarios with dense traffic (> 50 simultaneous tracks),
 *          replace the greedy assignment with scipy-style linear_sum_assignment
 *          or the Jonker-Volgenant algorithm (LAPJV).
 *
 * @author  Om Jagdale
 */

#include "tracker.hpp"
#include "config.hpp"
#include <algorithm>
#include <set>
#include <iostream>

namespace tracker {

    IoUTracker::IoUTracker() : next_id(1) {}

    // ─────────────────────────────────────────────────────────────────
    //  Kalman Filter (4-state: cx, cy, dx, dy)
    // ─────────────────────────────────────────────────────────────────

    void IoUTracker::init_kalman_filter(cv::KalmanFilter& kf, const cv::Rect& bbox) {
        kf.init(4, 2, 0);

        // State transition: constant velocity model
        //   x(t+1) = x(t) + dx(t)
        //   y(t+1) = y(t) + dy(t)
        kf.transitionMatrix = (cv::Mat_<float>(4, 4) << 
            1, 0, 1, 0,
            0, 1, 0, 1,
            0, 0, 1, 0,
            0, 0, 0, 1);
        
        // Observation: direct measurement of centroid (cx, cy)
        kf.measurementMatrix = (cv::Mat_<float>(2, 4) << 
            1, 0, 0, 0,
            0, 1, 0, 0);
            
        cv::setIdentity(kf.processNoiseCov, cv::Scalar::all(1e-2));
        cv::setIdentity(kf.measurementNoiseCov, cv::Scalar::all(1e-1));
        cv::setIdentity(kf.errorCovPost, cv::Scalar::all(1.0));

        // Initialise state with bounding box centroid, zero velocity
        kf.statePost.at<float>(0) = static_cast<float>(bbox.x + bbox.width / 2);
        kf.statePost.at<float>(1) = static_cast<float>(bbox.y + bbox.height / 2);
        kf.statePost.at<float>(2) = 0.0f;
        kf.statePost.at<float>(3) = 0.0f;
    }

    cv::Rect IoUTracker::predict_kalman(cv::KalmanFilter& kf) {
        cv::Mat prediction = kf.predict();
        int cx = static_cast<int>(prediction.at<float>(0));
        int cy = static_cast<int>(prediction.at<float>(1));
        return cv::Rect(cx, cy, 0, 0);  // Width/height preserved from track state
    }

    void IoUTracker::update_kalman(cv::KalmanFilter& kf, const cv::Rect& bbox) {
        cv::Mat measurement(2, 1, CV_32F);
        measurement.at<float>(0) = static_cast<float>(bbox.x + bbox.width / 2);
        measurement.at<float>(1) = static_cast<float>(bbox.y + bbox.height / 2);
        kf.correct(measurement);
    }

    // ─────────────────────────────────────────────────────────────────
    //  IoU Computation
    // ─────────────────────────────────────────────────────────────────

    double IoUTracker::compute_iou(const cv::Rect& a, const cv::Rect& b) {
        cv::Rect intersection = a & b;
        double inter_area = intersection.area();
        if (inter_area <= 0.0) return 0.0;
        
        double union_area = static_cast<double>(a.area()) + b.area() - inter_area;
        return (union_area > 0.0) ? (inter_area / union_area) : 0.0;
    }

    // ─────────────────────────────────────────────────────────────────
    //  Update (per-frame entry point)
    // ─────────────────────────────────────────────────────────────────

    std::vector<Track> IoUTracker::update(
        const std::vector<vehicle_detection::Detection>& detections
    ) {
        // ── Step 1: Predict existing track positions ─────────────────
        for (auto& [id, trk] : tracks) {
            trk.time_since_update++;
            trk.age++;
            
            if (config::USE_KALMAN) {
                cv::Rect kf_pred = predict_kalman(trk.kf);
                // Shift bbox centre to Kalman-predicted position
                trk.bbox.x = kf_pred.x - trk.bbox.width / 2;
                trk.bbox.y = kf_pred.y - trk.bbox.height / 2;
            }
        }

        // ── Step 2: Build candidate matches (IoU > threshold) ────────
        struct Match {
            int track_id;
            int det_idx;
            double iou;
        };
        std::vector<Match> candidates;
        candidates.reserve(tracks.size() * detections.size());

        for (const auto& [t_id, trk] : tracks) {
            for (int i = 0; i < static_cast<int>(detections.size()); ++i) {
                double iou = compute_iou(trk.bbox, detections[static_cast<size_t>(i)].bbox);
                if (iou > config::IOU_THRESHOLD) {
                    candidates.push_back({t_id, i, iou});
                }
            }
        }

        // Sort by descending IoU for greedy assignment
        std::sort(candidates.begin(), candidates.end(),
                  [](const Match& a, const Match& b) { return a.iou > b.iou; });

        // ── Step 3: Greedy assignment (IoU-sorted) ───────────────────
        std::set<int> assigned_tracks;
        std::set<int> assigned_dets;
        std::map<int, int> matches;  // track_id → det_idx

        for (const auto& m : candidates) {
            if (assigned_tracks.count(m.track_id) || assigned_dets.count(m.det_idx)) {
                continue;
            }
            matches[m.track_id] = m.det_idx;
            assigned_tracks.insert(m.track_id);
            assigned_dets.insert(m.det_idx);
        }

        // ── Step 4: Update matched tracks ────────────────────────────
        for (const auto& [t_id, d_idx] : matches) {
            Track& trk = tracks[t_id];
            const auto& det = detections[static_cast<size_t>(d_idx)];

            trk.time_since_update = 0;
            trk.hits++;
            trk.class_id = det.class_id;
            trk.label = det.label;

            // EMA bounding box smoothing
            const double alpha = config::BBOX_SMOOTH_ALPHA;
            trk.bbox.x      = static_cast<int>(alpha * det.bbox.x      + (1.0 - alpha) * trk.bbox.x);
            trk.bbox.y      = static_cast<int>(alpha * det.bbox.y      + (1.0 - alpha) * trk.bbox.y);
            trk.bbox.width  = static_cast<int>(alpha * det.bbox.width  + (1.0 - alpha) * trk.bbox.width);
            trk.bbox.height = static_cast<int>(alpha * det.bbox.height + (1.0 - alpha) * trk.bbox.height);

            if (config::USE_KALMAN) {
                update_kalman(trk.kf, trk.bbox);
            }
        }

        // ── Step 5: Create new tracks for unmatched detections ───────
        for (int i = 0; i < static_cast<int>(detections.size()); ++i) {
            if (assigned_dets.count(i)) continue;

            Track new_trk;
            new_trk.track_id = next_id++;
            new_trk.bbox = detections[static_cast<size_t>(i)].bbox;
            new_trk.class_id = detections[static_cast<size_t>(i)].class_id;
            new_trk.label = detections[static_cast<size_t>(i)].label;
            new_trk.age = 1;
            new_trk.hits = 1;
            new_trk.time_since_update = 0;
            
            if (config::USE_KALMAN) {
                init_kalman_filter(new_trk.kf, new_trk.bbox);
            }
            
            tracks[new_trk.track_id] = std::move(new_trk);
        }

        // ── Step 6: Prune dead tracks ────────────────────────────────
        for (auto it = tracks.begin(); it != tracks.end(); ) {
            if (it->second.time_since_update > config::MAX_AGE) {
                it = tracks.erase(it);
            } else {
                ++it;
            }
        }

        // ── Step 7: Return confirmed tracks ──────────────────────────
        std::vector<Track> active;
        active.reserve(tracks.size());
        for (const auto& [id, trk] : tracks) {
            if (trk.hits >= config::MIN_HITS || trk.age <= 1) {
                active.push_back(trk);
            }
        }

        return active;
    }

}
