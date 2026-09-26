#include "preprocessing.hpp"
#include <opencv2/imgproc.hpp>
#include <algorithm>

namespace preprocessing {

    cv::Mat resize_frame(const cv::Mat& frame, int width, int height) {
        cv::Mat resized;
        cv::resize(frame, resized, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
        return resized;
    }

    cv::Mat to_gray(const cv::Mat& frame) {
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        return gray;
    }
    
    cv::Mat apply_clahe(const cv::Mat& gray, double clip_limit, int tile_size) {
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(clip_limit, cv::Size(tile_size, tile_size));
        cv::Mat enhanced;
        clahe->apply(gray, enhanced);
        return enhanced;
    }

    cv::Mat apply_gaussian_blur(const cv::Mat& frame, int kernel_size) {
        cv::Mat blurred;
        cv::GaussianBlur(frame, blurred, cv::Size(kernel_size, kernel_size), 0);
        return blurred;
    }

    cv::Mat adaptive_canny(const cv::Mat& gray) {
        double low = 50.0, high = 150.0;
        
        if (config::USE_ADAPTIVE_CANNY) {
            cv::Mat thresh;
            double otsu_thresh = cv::threshold(gray, thresh, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
            low = std::max(otsu_thresh * 0.5, 30.0);
            high = std::max(otsu_thresh * 1.0, 80.0);
        }
        
        cv::Mat edges;
        cv::Canny(gray, edges, low, high);
        return edges;
    }

    cv::Mat get_roi_mask(const cv::Mat& frame) {
        int h = frame.rows;
        int w = frame.cols;
        
        cv::Point pts[1][4];
        pts[0][0] = cv::Point(w * config::ROI_BOTTOM_LEFT_X,  h * config::ROI_BOTTOM_Y);
        pts[0][1] = cv::Point(w * config::ROI_TOP_LEFT_X,     h * config::ROI_TOP_Y);
        pts[0][2] = cv::Point(w * config::ROI_TOP_RIGHT_X,    h * config::ROI_TOP_Y);
        pts[0][3] = cv::Point(w * config::ROI_BOTTOM_RIGHT_X, h * config::ROI_BOTTOM_Y);
        
        const cv::Point* ppt[1] = { pts[0] };
        int npt[] = { 4 };
        
        cv::Mat mask = cv::Mat::zeros(frame.size(), CV_8UC1);
        cv::fillPoly(mask, ppt, npt, 1, cv::Scalar(255));
        
        return mask;
    }

    cv::Mat apply_roi(const cv::Mat& frame, const cv::Mat& mask) {
        cv::Mat result;
        cv::bitwise_and(frame, frame, result, mask);
        return result;
    }

    cv::Mat preprocess_for_detection(const cv::Mat& frame) {
        return resize_frame(frame);
    }

    cv::Mat preprocess_for_lane(const cv::Mat& frame) {
        cv::Mat resized = resize_frame(frame);
        cv::Mat gray = to_gray(resized);
        cv::Mat enhanced = apply_clahe(gray);
        cv::Mat blurred = apply_gaussian_blur(enhanced);
        cv::Mat edges = adaptive_canny(blurred);
        cv::Mat mask = get_roi_mask(edges);
        return apply_roi(edges, mask);
    }

}
