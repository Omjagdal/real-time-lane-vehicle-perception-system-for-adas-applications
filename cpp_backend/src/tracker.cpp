#include "tracker.hpp"
#include "config.hpp"
#include <algorithm>
#include <iostream>

namespace tracker {

    IoUTracker::IoUTracker() : next_id(1) {}

    void IoUTracker::init_kalman_filter(cv::KalmanFilter& kf, const cv::Rect& bbox) {
        kf.init(4, 2, 0);
        // Transition matrix A
        kf.transitionMatrix = (cv::Mat_<float>(4, 4) << 
            1, 0, 1, 0,
            0, 1, 0, 1,
            0, 0, 1, 0,
            0, 0, 0, 1);
        
        // Measurement matrix H
        kf.measurementMatrix = (cv::Mat_<float>(2, 4) << 
            1, 0, 0, 0,
            0, 1, 0, 0);
            
        cv::setIdentity(kf.processNoiseCov, cv::Scalar::all(1e-2));
        cv::setIdentity(kf.measurementNoiseCov, cv::Scalar::all(1e-1));
        cv::setIdentity(kf.errorCovPost, cv::Scalar::all(1.0));

        kf.statePost.at<float>(0) = bbox.x + bbox.width / 2.0f;
        kf.statePost.at<float>(1) = bbox.y + bbox.height / 2.0f;
        kf.statePost.at<float>(2) = 0.0f;
        kf.statePost.at<float>(3) = 0.0f;
    }

    cv::Rect IoUTracker::predict_kalman(cv::KalmanFilter& kf) {
        cv::Mat prediction = kf.predict();
        float cx = prediction.at<float>(0);
        float cy = prediction.at<float>(1);
        // We do not model width/height in this simple KF, so we will combine this 
        // with the tracker's stored bbox later.
        return cv::Rect(cx, cy, 0, 0); // Temporary return
    }

    void IoUTracker::update_kalman(cv::KalmanFilter& kf, const cv::Rect& bbox) {
        cv::Mat measurement(2, 1, CV_32F);
        measurement.at<float>(0) = bbox.x + bbox.width / 2.0f;
        measurement.at<float>(1) = bbox.y + bbox.height / 2.0f;
        kf.correct(measurement);
    }

    double IoUTracker::compute_iou(const cv::Rect& a, const cv::Rect& b) {
        cv::Rect inter = a & b;
        double inter_area = inter.area();
        if (inter_area == 0) return 0.0;
        
        double union_area = a.area() + b.area() - inter_area;
        return inter_area / union_area;
    }

    std::vector<Track> IoUTracker::update(const std::vector<vehicle_detection::Detection>& detections) {
        
        // 1. Predict
        for (auto& pair : tracks) {
            Track& trk = pair.second;
            trk.time_since_update++;
            trk.age++;
            
            if (config::USE_KALMAN) {
                cv::Rect kf_pred = predict_kalman(trk.kf);
                // Update center based on KF prediction
                trk.bbox.x = kf_pred.x - trk.bbox.width / 2;
                trk.bbox.y = kf_pred.y - trk.bbox.height / 2;
            }
        }

        // 2. Compute cost matrix & Greedy Assignment
        std::vector<int> unassigned_dets;
        for (int i = 0; i < detections.size(); ++i) unassigned_dets.push_back(i);
        
        std::vector<int> unassigned_tracks;
        for (const auto& pair : tracks) unassigned_tracks.push_back(pair.first);

        struct Match {
            int trk_id;
            int det_idx;
            double iou;
        };
        std::vector<Match> potential_matches;

        for (int t_id : unassigned_tracks) {
            for (int i = 0; i < detections.size(); ++i) {
                double iou = compute_iou(tracks[t_id].bbox, detections[i].bbox);
                if (iou > config::IOU_THRESHOLD) {
                    potential_matches.push_back({t_id, i, iou});
                }
            }
        }

        // Sort by highest IoU first
        std::sort(potential_matches.begin(), potential_matches.end(), [](const Match& a, const Match& b) {
            return a.iou > b.iou;
        });

        std::map<int, int> matched_tracks; // trk_id -> det_idx
        for (const auto& m : potential_matches) {
            if (matched_tracks.find(m.trk_id) == matched_tracks.end()) {
                // Check if det_idx is already taken
                bool taken = false;
                for (const auto& pair : matched_tracks) {
                    if (pair.second == m.det_idx) { taken = true; break; }
                }
                if (!taken) {
                    matched_tracks[m.trk_id] = m.det_idx;
                }
            }
        }

        // 3. Update Matched
        for (const auto& pair : matched_tracks) {
            int t_id = pair.first;
            int d_idx = pair.second;
            Track& trk = tracks[t_id];
            const auto& det = detections[d_idx];

            trk.time_since_update = 0;
            trk.hits++;
            trk.class_id = det.class_id;
            trk.label = det.label;

            // EMA Smoothing
            trk.bbox.x = config::BBOX_SMOOTH_ALPHA * det.bbox.x + (1 - config::BBOX_SMOOTH_ALPHA) * trk.bbox.x;
            trk.bbox.y = config::BBOX_SMOOTH_ALPHA * det.bbox.y + (1 - config::BBOX_SMOOTH_ALPHA) * trk.bbox.y;
            trk.bbox.width = config::BBOX_SMOOTH_ALPHA * det.bbox.width + (1 - config::BBOX_SMOOTH_ALPHA) * trk.bbox.width;
            trk.bbox.height = config::BBOX_SMOOTH_ALPHA * det.bbox.height + (1 - config::BBOX_SMOOTH_ALPHA) * trk.bbox.height;

            if (config::USE_KALMAN) {
                update_kalman(trk.kf, trk.bbox);
            }
        }

        // 4. Create New Tracks
        for (int i = 0; i < detections.size(); ++i) {
            bool matched = false;
            for (const auto& pair : matched_tracks) {
                if (pair.second == i) { matched = true; break; }
            }
            if (!matched) {
                Track new_trk;
                new_trk.track_id = next_id++;
                new_trk.bbox = detections[i].bbox;
                new_trk.class_id = detections[i].class_id;
                new_trk.label = detections[i].label;
                new_trk.age = 1;
                new_trk.hits = 1;
                new_trk.time_since_update = 0;
                
                if (config::USE_KALMAN) {
                    init_kalman_filter(new_trk.kf, new_trk.bbox);
                }
                
                tracks[new_trk.track_id] = new_trk;
            }
        }

        // 5. Delete Dead Tracks
        std::vector<int> to_delete;
        for (const auto& pair : tracks) {
            if (pair.second.time_since_update > config::MAX_AGE) {
                to_delete.push_back(pair.first);
            }
        }
        for (int t_id : to_delete) {
            tracks.erase(t_id);
        }

        // 6. Return Active Tracks
        std::vector<Track> active_tracks;
        for (const auto& pair : tracks) {
            if (pair.second.hits >= config::MIN_HITS || pair.second.age <= 1) {
                active_tracks.push_back(pair.second);
            }
        }

        return active_tracks;
    }

}
