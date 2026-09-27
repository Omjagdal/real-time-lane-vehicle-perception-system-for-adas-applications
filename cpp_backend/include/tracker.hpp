/**
 * @file    tracker.hpp
 * @brief   IoU-based multi-object tracker with Kalman filter motion prediction.
 *
 * Implements a frame-by-frame multi-object tracker that associates incoming
 * detections with existing tracks using an IoU cost matrix solved by the
 * Hungarian algorithm. Each track maintains a Kalman filter for state
 * prediction between frames, improving robustness during brief occlusions.
 *
 * Track lifecycle:
 *   - **Tentative**: Newly created from an unmatched detection.
 *     Promoted to confirmed after `MIN_HITS` consecutive matches.
 *   - **Confirmed**: Actively tracked. Returned in the update() output.
 *   - **Deleted**: Removed after `MAX_AGE` frames without a match.
 *
 * @see     config.hpp for IOU_THRESHOLD, MAX_AGE, MIN_HITS, USE_KALMAN.
 * @author  Om Jagdale
 */

#pragma once

#include <vector>
#include <map>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/video/tracking.hpp>
#include "vehicle_detection.hpp"

namespace tracker {

    /**
     * @brief Represents a single tracked object across frames.
     *
     * Each track holds a unique persistent ID, the current bounding box
     * (smoothed), the associated class label, and lifecycle counters.
     */
    struct Track {
        int track_id;               ///< Unique, monotonically increasing identifier.
        cv::Rect bbox;              ///< Current bounding box (EMA-smoothed).
        int class_id;               ///< COCO class ID of the tracked object.
        std::string label;          ///< Human-readable class name.
        int age;                    ///< Total number of frames since track creation.
        int hits;                   ///< Total number of successful detection matches.
        int time_since_update;      ///< Frames elapsed since the last successful match.
        cv::KalmanFilter kf;        ///< 7-state linear Kalman filter for motion prediction.
    };

    /**
     * @brief Multi-object tracker using IoU association and Kalman prediction.
     *
     * Usage:
     * @code
     *   tracker::IoUTracker trk;
     *   for (auto& frame : video) {
     *       auto detections = detector.detect(frame);
     *       auto tracks = trk.update(detections);
     *       // tracks contain persistent IDs across frames
     *   }
     * @endcode
     */
    class IoUTracker {
    public:
        /** @brief Construct the tracker with default parameters from config.hpp. */
        IoUTracker();

        /**
         * @brief Update tracks with new detections from the current frame.
         *
         * Steps:
         *   1. Predict all existing track positions via Kalman filter.
         *   2. Compute IoU cost matrix between predictions and detections.
         *   3. Solve optimal assignment using the Hungarian algorithm.
         *   4. Update matched tracks; increment age of unmatched tracks.
         *   5. Create new tracks for unmatched detections.
         *   6. Delete tracks exceeding MAX_AGE without a match.
         *
         * @param detections    Current frame's detection results.
         * @return              Vector of confirmed tracks (hits >= MIN_HITS).
         */
        std::vector<Track> update(const std::vector<vehicle_detection::Detection>& detections);

    private:
        int next_id;                    ///< Next available track ID (monotonically increasing).
        std::map<int, Track> tracks;    ///< Active track pool keyed by track_id.

        /**
         * @brief Initialise a 7-state Kalman filter for bounding box tracking.
         *
         * State vector: [cx, cy, area, aspect_ratio, dx, dy, d_area].
         *
         * @param kf    Kalman filter to initialise (modified in place).
         * @param bbox  Initial bounding box for state initialisation.
         */
        void init_kalman_filter(cv::KalmanFilter& kf, const cv::Rect& bbox);

        /**
         * @brief Predict the next bounding box position using the Kalman filter.
         * @param kf    Kalman filter to predict from.
         * @return      Predicted bounding box.
         */
        cv::Rect predict_kalman(cv::KalmanFilter& kf);

        /**
         * @brief Correct the Kalman filter state with a matched detection.
         * @param kf    Kalman filter to update.
         * @param bbox  Matched detection bounding box (measurement).
         */
        void update_kalman(cv::KalmanFilter& kf, const cv::Rect& bbox);

        /**
         * @brief Compute Intersection-over-Union between two bounding boxes.
         * @param a     First bounding box.
         * @param b     Second bounding box.
         * @return      IoU value in [0.0, 1.0].
         */
        double compute_iou(const cv::Rect& a, const cv::Rect& b);
    };

}
