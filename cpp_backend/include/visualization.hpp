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

    cv::Mat draw_lanes(cv::Mat& frame, 
                       const lane_detection::Lines& left_line, 
                       const lane_detection::Lines& right_line, 
                       const std::string& departure_status);

    cv::Mat draw_vehicles(cv::Mat& frame, 
                          const std::vector<tracker::Track>& tracks, 
                          const std::map<int, double>& distances, 
                          const std::map<int, double>& speeds, 
                          const std::map<int, estimators::FCWResult>& fcw_results);

    cv::Mat draw_fcw_banner(cv::Mat& frame, const std::string& alert_level);

    cv::Mat draw_hud(cv::Mat& frame, 
                     double fps, 
                     int num_tracks, 
                     double ego_speed_kmh, 
                     std::tuple<float, float> lane_confidence, 
                     const std::string& departure_status);

    cv::Mat draw_minimap(cv::Mat& frame, 
                         const std::vector<tracker::Track>& tracks, 
                         const std::map<int, double>& distances, 
                         const std::map<int, estimators::FCWResult>& fcw_results);

}
