/**
 * @file    lane_detection.hpp
 * @brief   Lane detection module using Probabilistic Hough Transform with temporal smoothing.
 *
 * Implements a classical computer vision pipeline for detecting left and
 * right lane boundaries from edge-detected road images. The detector
 * maintains internal state for temporal smoothing and lane departure
 * monitoring.
 *
 * Processing stages:
 *   1. Probabilistic Hough Transform on the edge/ROI image.
 *   2. Slope-based classification into left (negative) and right (positive) groups.
 *   3. Statistical outlier rejection (1.5-sigma from mean slope).
 *   4. Length-weighted parameter averaging per side.
 *   5. Temporal smoothing via sliding-window EMA.
 *   6. Lane departure detection based on midpoint offset.
 *
 * @note    This module uses classical CV only. For curved or multi-lane
 *          roads, consider replacing with a learned lane detector
 *          (e.g., LaneATT, CLRNet).
 *
 * @see     config.hpp for Hough and smoothing parameters.
 * @author  Om Jagdale
 */

#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <deque>
#include <string>
#include <tuple>
#include <optional>

namespace lane_detection {

    /** @brief A single line segment represented as [x1, y1, x2, y2]. */
    using Line = cv::Vec4i;

    /**
     * @brief Optional collection of line segments.
     * `std::nullopt` indicates that no lane was detected on that side.
     */
    using Lines = std::optional<std::vector<Line>>;

    /**
     * @brief Stateful lane detector with temporal smoothing and departure monitoring.
     *
     * Maintains per-side history buffers for slope/intercept smoothing and
     * tracks lane departure status across frames.
     */
    class LaneDetector {
    public:
        /**
         * @brief Construct a LaneDetector with the given smoothing window.
         * @param smooth_window     Number of past frames for temporal averaging
         *                          (default: config::LANE_SMOOTH_WINDOW).
         */
        LaneDetector(int smooth_window = -1);

        /**
         * @brief Detect left and right lane lines from an edge image.
         *
         * @param edge_image    Binary edge map (output of preprocessing::preprocess_for_lane).
         * @param frame_height  Height of the original frame (for line extrapolation).
         * @param frame_width   Width of the original frame (for midpoint computation).
         * @return              Tuple of (left_lane, right_lane), each as optional Lines.
         */
        std::tuple<Lines, Lines> detect(const cv::Mat& edge_image, 
                                        int frame_height, 
                                        int frame_width);

        /**
         * @brief Get the current detection confidence for each lane.
         * @return Tuple of (left_confidence, right_confidence) in [0.0, 1.0].
         */
        std::tuple<float, float> get_confidence() const;

        /**
         * @brief Get the current lane departure status.
         * @return One of: "CENTERED", "LEFT_DEPARTURE", "RIGHT_DEPARTURE".
         */
        std::string get_departure_status() const;

        /** @brief Reset all internal state (history buffers, confidence, departure). */
        void reset();

    private:
        int window_size;                                            ///< Temporal smoothing window length.
        std::deque<std::tuple<double, double>> left_history;        ///< Left lane (slope, intercept) history.
        std::deque<std::tuple<double, double>> right_history;       ///< Right lane (slope, intercept) history.
        float left_confidence;                                      ///< Left lane detection confidence.
        float right_confidence;                                     ///< Right lane detection confidence.
        std::string departure_status;                               ///< Current departure state.

        /**
         * @brief Run Probabilistic Hough Transform on the edge image.
         * @param edge_image    Binary edge map.
         * @return              Raw line segments as Vec4i vectors.
         */
        std::vector<cv::Vec4i> hough_lines(const cv::Mat& edge_image);

        /**
         * @brief Classify raw line segments into left and right groups by slope.
         * @param raw_lines     Output of hough_lines().
         * @return              Tuple of (left_params, right_params), each as
         *                      vectors of (slope, intercept, length).
         */
        std::tuple<std::vector<std::tuple<double, double, double>>, 
                   std::vector<std::tuple<double, double, double>>> 
        split_lines(const std::vector<cv::Vec4i>& raw_lines);

        /**
         * @brief Remove slope outliers beyond sigma_factor standard deviations.
         * @param params            Line parameters (slope, intercept, length).
         * @param sigma_factor      Outlier rejection threshold (default: 1.5).
         * @return                  Filtered parameter vector.
         */
        std::vector<std::tuple<double, double, double>> reject_outliers(
            const std::vector<std::tuple<double, double, double>>& params, 
            double sigma_factor = 1.5);

        /**
         * @brief Compute detection confidence from the number and consistency of segments.
         * @param params    Filtered line parameters.
         * @return          Confidence score in [0.0, 1.0].
         */
        float compute_confidence(const std::vector<std::tuple<double, double, double>>& params);

        /**
         * @brief Compute length-weighted average slope and intercept.
         * @param params    Line parameters with lengths as weights.
         * @return          Optional (slope, intercept), or nullopt if params is empty.
         */
        std::optional<std::tuple<double, double>> weighted_average(
            const std::vector<std::tuple<double, double, double>>& params);

        /**
         * @brief Build an extrapolated lane line with temporal smoothing.
         * @param params        Current frame's filtered parameters.
         * @param frame_height  Frame height for Y-coordinate extrapolation.
         * @param frame_width   Frame width for X-coordinate clamping.
         * @param history       Reference to the side's history deque (modified in place).
         * @param side          "left" or "right" (for logging/debugging).
         * @return              Optional line segments, or nullopt if unavailable.
         */
        Lines make_line(const std::vector<std::tuple<double, double, double>>& params,
                        int frame_height, int frame_width,
                        std::deque<std::tuple<double, double>>& history,
                        const std::string& side);

        /**
         * @brief Generate fallback lane lines when detection fails completely.
         * @param frame_height  Frame height.
         * @param frame_width   Frame width.
         * @return              Default left and right lines based on frame geometry.
         */
        std::tuple<Lines, Lines> fallback_lines(int frame_height, int frame_width);

        /**
         * @brief Update the lane departure status based on detected lane positions.
         * @param left_line     Detected left lane (may be nullopt).
         * @param right_line    Detected right lane (may be nullopt).
         * @param frame_width   Frame width for midpoint calculation.
         */
        void update_departure(const Lines& left_line, const Lines& right_line, int frame_width);
    };

}
