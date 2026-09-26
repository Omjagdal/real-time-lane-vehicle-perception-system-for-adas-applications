#pragma once

#include <vector>
#include <map>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/video/tracking.hpp>
#include "vehicle_detection.hpp"

namespace tracker {

    struct Track {
        int track_id;
        cv::Rect bbox;
        int class_id;
        std::string label;
        int age;
        int hits;
        int time_since_update;
        
        cv::KalmanFilter kf;
    };

    class IoUTracker {
    public:
        IoUTracker();
        std::vector<Track> update(const std::vector<vehicle_detection::Detection>& detections);
    private:
        int next_id;
        std::map<int, Track> tracks;
        
        void init_kalman_filter(cv::KalmanFilter& kf, const cv::Rect& bbox);
        cv::Rect predict_kalman(cv::KalmanFilter& kf);
        void update_kalman(cv::KalmanFilter& kf, const cv::Rect& bbox);
        
        double compute_iou(const cv::Rect& a, const cv::Rect& b);
    };

}
