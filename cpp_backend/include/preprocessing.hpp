#pragma once

#include <opencv2/opencv.hpp>
#include "config.hpp"

namespace preprocessing {

    cv::Mat resize_frame(const cv::Mat& frame, 
                         int width = config::TARGET_WIDTH, 
                         int height = config::TARGET_HEIGHT);

    cv::Mat to_gray(const cv::Mat& frame);
    
    cv::Mat apply_clahe(const cv::Mat& gray, 
                        double clip_limit = config::CLAHE_CLIP_LIMIT, 
                        int tile_size = config::CLAHE_TILE_SIZE);

    cv::Mat apply_gaussian_blur(const cv::Mat& frame, 
                                int kernel_size = config::GAUSSIAN_KERNEL);

    cv::Mat adaptive_canny(const cv::Mat& gray);

    cv::Mat get_roi_mask(const cv::Mat& frame);

    cv::Mat apply_roi(const cv::Mat& frame, const cv::Mat& mask);

    cv::Mat preprocess_for_detection(const cv::Mat& frame);

    cv::Mat preprocess_for_lane(const cv::Mat& frame);

}
