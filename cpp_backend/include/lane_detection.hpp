#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <deque>
#include <string>
#include <tuple>
#include <optional>

namespace lane_detection {

    using Line = cv::Vec4i; // [x1, y1, x2, y2]
    using Lines = std::optional<std::vector<Line>>;

    class LaneDetector {
    public:
        LaneDetector(int smooth_window = -1);

        std::tuple<Lines, Lines> detect(const cv::Mat& edge_image, 
                                        int frame_height, 
                                        int frame_width);

        std::tuple<float, float> get_confidence() const;
        std::string get_departure_status() const;
        
        void reset();

    private:
        int window_size;
        std::deque<std::tuple<double, double>> left_history;
        std::deque<std::tuple<double, double>> right_history;
        float left_confidence;
        float right_confidence;
        std::string departure_status;

        std::vector<cv::Vec4i> hough_lines(const cv::Mat& edge_image);
        
        std::tuple<std::vector<std::tuple<double, double, double>>, 
                   std::vector<std::tuple<double, double, double>>> 
        split_lines(const std::vector<cv::Vec4i>& raw_lines);

        std::vector<std::tuple<double, double, double>> reject_outliers(
            const std::vector<std::tuple<double, double, double>>& params, 
            double sigma_factor = 1.5);

        float compute_confidence(const std::vector<std::tuple<double, double, double>>& params);

        std::optional<std::tuple<double, double>> weighted_average(
            const std::vector<std::tuple<double, double, double>>& params);

        Lines make_line(const std::vector<std::tuple<double, double, double>>& params,
                        int frame_height, int frame_width,
                        std::deque<std::tuple<double, double>>& history,
                        const std::string& side);

        std::tuple<Lines, Lines> fallback_lines(int frame_height, int frame_width);

        void update_departure(const Lines& left_line, const Lines& right_line, int frame_width);
    };

}
