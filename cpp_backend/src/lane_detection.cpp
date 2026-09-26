#include "lane_detection.hpp"
#include "config.hpp"
#include <numeric>
#include <cmath>
#include <algorithm>

namespace lane_detection {

    LaneDetector::LaneDetector(int smooth_window) {
        window_size = (smooth_window > 0) ? smooth_window : config::LANE_SMOOTH_WINDOW;
        left_confidence = 0.0f;
        right_confidence = 0.0f;
        departure_status = "centred";
    }

    void LaneDetector::reset() {
        left_history.clear();
        right_history.clear();
        left_confidence = 0.0f;
        right_confidence = 0.0f;
        departure_status = "centred";
    }

    std::tuple<float, float> LaneDetector::get_confidence() const {
        return {left_confidence, right_confidence};
    }

    std::string LaneDetector::get_departure_status() const {
        return departure_status;
    }

    std::vector<cv::Vec4i> LaneDetector::hough_lines(const cv::Mat& edge_image) {
        std::vector<cv::Vec4i> lines;
        cv::HoughLinesP(edge_image, lines, config::HOUGH_RHO, CV_PI / 180.0,
                        config::HOUGH_THRESHOLD, config::HOUGH_MIN_LEN, config::HOUGH_MAX_GAP);
        return lines;
    }

    std::tuple<std::vector<std::tuple<double, double, double>>, 
               std::vector<std::tuple<double, double, double>>> 
    LaneDetector::split_lines(const std::vector<cv::Vec4i>& raw_lines) {
        
        std::vector<std::tuple<double, double, double>> left_params;
        std::vector<std::tuple<double, double, double>> right_params;

        for (const auto& line : raw_lines) {
            double x1 = line[0], y1 = line[1], x2 = line[2], y2 = line[3];
            if (x2 == x1) continue;
            
            double slope = (y2 - y1) / (x2 - x1);
            if (std::abs(slope) < config::SLOPE_MIN || std::abs(slope) > config::SLOPE_MAX) continue;
            
            double intercept = y1 - slope * x1;
            double length = std::hypot(x2 - x1, y2 - y1);

            if (slope < 0) {
                left_params.emplace_back(slope, intercept, length);
            } else {
                right_params.emplace_back(slope, intercept, length);
            }
        }
        return {left_params, right_params};
    }

    std::vector<std::tuple<double, double, double>> LaneDetector::reject_outliers(
        const std::vector<std::tuple<double, double, double>>& params, 
        double sigma_factor) {
        
        if (params.size() < 3) return params;

        std::vector<double> slopes;
        for (const auto& p : params) slopes.push_back(std::get<0>(p));
        
        std::vector<double> sorted_slopes = slopes;
        std::sort(sorted_slopes.begin(), sorted_slopes.end());
        double median = sorted_slopes[sorted_slopes.size() / 2];

        double sum = std::accumulate(slopes.begin(), slopes.end(), 0.0);
        double mean = sum / slopes.size();
        
        double sq_sum = std::inner_product(slopes.begin(), slopes.end(), slopes.begin(), 0.0);
        double stddev = std::sqrt(sq_sum / slopes.size() - mean * mean);

        if (stddev < 0.01) return params;

        std::vector<std::tuple<double, double, double>> filtered;
        for (const auto& p : params) {
            if (std::abs(std::get<0>(p) - median) <= sigma_factor * stddev) {
                filtered.push_back(p);
            }
        }
        return filtered;
    }

    float LaneDetector::compute_confidence(const std::vector<std::tuple<double, double, double>>& params) {
        if (params.empty()) return 0.0f;
        
        int n = params.size();
        std::vector<double> slopes;
        for (const auto& p : params) slopes.push_back(std::get<0>(p));
        
        double stddev = 0.5;
        if (n > 1) {
            double sum = std::accumulate(slopes.begin(), slopes.end(), 0.0);
            double mean = sum / n;
            double sq_sum = std::inner_product(slopes.begin(), slopes.end(), slopes.begin(), 0.0);
            stddev = std::sqrt(sq_sum / n - mean * mean);
        }

        double count_score = std::min(n / 8.0, 1.0);
        double consistency_score = std::max(1.0 - stddev, 0.0);
        
        return std::round((0.6 * count_score + 0.4 * consistency_score) * 1000.0) / 1000.0;
    }

    std::optional<std::tuple<double, double>> LaneDetector::weighted_average(
        const std::vector<std::tuple<double, double, double>>& params) {
        
        if (params.size() < config::LANE_MIN_SEGMENTS) return std::nullopt;
        
        double total_length = 0.0;
        double sum_slope = 0.0;
        double sum_intercept = 0.0;

        for (const auto& p : params) {
            double length = std::get<2>(p);
            total_length += length;
            sum_slope += std::get<0>(p) * length;
            sum_intercept += std::get<1>(p) * length;
        }

        if (total_length < 1e-6) return std::nullopt;
        
        return std::make_tuple(sum_slope / total_length, sum_intercept / total_length);
    }

    Lines LaneDetector::make_line(const std::vector<std::tuple<double, double, double>>& params,
                                  int frame_height, int frame_width,
                                  std::deque<std::tuple<double, double>>& history,
                                  const std::string& side) {
        
        auto result = weighted_average(params);
        if (result.has_value()) {
            history.push_back(result.value());
            if (history.size() > window_size) {
                history.pop_front();
            }
        }

        if (history.empty()) return std::nullopt;

        double alpha = config::LANE_EMA_ALPHA;
        std::vector<double> weights(history.size());
        double weight_sum = 0.0;
        for (int i = 0; i < history.size(); ++i) {
            weights[i] = alpha * std::pow(1 - alpha, history.size() - 1 - i);
            weight_sum += weights[i];
        }

        double final_slope = 0.0;
        double final_intercept = 0.0;
        for (int i = 0; i < history.size(); ++i) {
            final_slope += std::get<0>(history[i]) * (weights[i] / weight_sum);
            final_intercept += std::get<1>(history[i]) * (weights[i] / weight_sum);
        }

        if (std::abs(final_slope) < 1e-6) return std::nullopt;

        int y_bottom = static_cast<int>(frame_height * config::ROI_BOTTOM_Y);
        int y_top    = static_cast<int>(frame_height * config::ROI_TOP_Y);

        int x_bottom = static_cast<int>((y_bottom - final_intercept) / final_slope);
        int x_top    = static_cast<int>((y_top - final_intercept) / final_slope);

        x_bottom = std::clamp(x_bottom, 0, frame_width - 1);
        x_top    = std::clamp(x_top, 0, frame_width - 1);

        std::vector<cv::Vec4i> line = {{x_bottom, y_bottom, x_top, y_top}};
        return line;
    }

    std::tuple<Lines, Lines> LaneDetector::fallback_lines(int frame_height, int frame_width) {
        Lines left = make_line({}, frame_height, frame_width, left_history, "left");
        Lines right = make_line({}, frame_height, frame_width, right_history, "right");
        return {left, right};
    }

    void LaneDetector::update_departure(const Lines& left_line, const Lines& right_line, int frame_width) {
        if (!left_line.has_value() || !right_line.has_value()) {
            departure_status = "centred";
            return;
        }

        double left_x = left_line.value()[0][0];
        double right_x = right_line.value()[0][0];
        
        double lane_centre = (left_x + right_x) / 2.0;
        double frame_centre = frame_width / 2.0;
        
        double offset_ratio = (frame_centre - lane_centre) / std::max(right_x - left_x, 1.0);

        if (offset_ratio > 0.25) departure_status = "right";
        else if (offset_ratio < -0.25) departure_status = "left";
        else departure_status = "centred";
    }

    std::tuple<Lines, Lines> LaneDetector::detect(const cv::Mat& edge_image, 
                                                  int frame_height, 
                                                  int frame_width) {
        auto raw_lines = hough_lines(edge_image);
        if (raw_lines.empty()) return fallback_lines(frame_height, frame_width);

        auto [left_params, right_params] = split_lines(raw_lines);
        
        left_params = reject_outliers(left_params);
        right_params = reject_outliers(right_params);

        left_confidence = compute_confidence(left_params);
        right_confidence = compute_confidence(right_params);

        Lines left_line = make_line(left_params, frame_height, frame_width, left_history, "left");
        Lines right_line = make_line(right_params, frame_height, frame_width, right_history, "right");

        update_departure(left_line, right_line, frame_width);

        return {left_line, right_line};
    }

}
