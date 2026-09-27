/**
 * @file    preprocessing.hpp
 * @brief   Image preprocessing pipeline for the ADAS perception system.
 *
 * Provides a set of stateless functions that transform raw camera frames
 * into formats suitable for lane detection and vehicle detection modules.
 *
 * The preprocessing pipeline consists of:
 *   1. Spatial normalisation (resize to TARGET_WIDTH x TARGET_HEIGHT)
 *   2. Contrast enhancement (CLAHE on luminance channel)
 *   3. Noise suppression (Gaussian blur)
 *   4. Edge extraction (adaptive Canny with Otsu-based thresholds)
 *   5. Region-of-interest masking (trapezoidal road area)
 *
 * @note    All functions are thread-safe (no shared mutable state).
 * @author  Om Jagdale
 */

#pragma once

#include <opencv2/opencv.hpp>
#include "config.hpp"

namespace preprocessing {

    /**
     * @brief Resize a frame to the target resolution using bilinear interpolation.
     * @param frame     Input BGR image (any resolution).
     * @param width     Target width in pixels (default: config::TARGET_WIDTH).
     * @param height    Target height in pixels (default: config::TARGET_HEIGHT).
     * @return          Resized BGR image.
     */
    cv::Mat resize_frame(const cv::Mat& frame, 
                         int width = config::TARGET_WIDTH, 
                         int height = config::TARGET_HEIGHT);

    /**
     * @brief Convert a BGR frame to single-channel grayscale.
     * @param frame     Input BGR image.
     * @return          Grayscale image (CV_8UC1).
     */
    cv::Mat to_gray(const cv::Mat& frame);

    /**
     * @brief Apply Contrast Limited Adaptive Histogram Equalization (CLAHE).
     * @param gray          Input grayscale image.
     * @param clip_limit    Contrast clipping threshold (default: 2.0).
     * @param tile_size     Tile grid dimension for local histogram (default: 8).
     * @return              Contrast-enhanced grayscale image.
     */
    cv::Mat apply_clahe(const cv::Mat& gray, 
                        double clip_limit = config::CLAHE_CLIP_LIMIT, 
                        int tile_size = config::CLAHE_TILE_SIZE);

    /**
     * @brief Apply Gaussian blur for noise reduction.
     * @param frame         Input image (grayscale or colour).
     * @param kernel_size   Square kernel dimension, must be odd (default: 5).
     * @return              Blurred image.
     */
    cv::Mat apply_gaussian_blur(const cv::Mat& frame, 
                                int kernel_size = config::GAUSSIAN_KERNEL);

    /**
     * @brief Compute edge map using adaptive Canny with Otsu-based thresholds.
     *
     * Uses Otsu's method on the grayscale histogram to automatically derive
     * the low and high thresholds, producing robust edges across varying
     * illumination conditions.
     *
     * @param gray  Input grayscale image.
     * @return      Binary edge map (CV_8UC1).
     */
    cv::Mat adaptive_canny(const cv::Mat& gray);

    /**
     * @brief Generate a trapezoidal ROI mask covering the road surface.
     *
     * The mask vertices are defined as fractions of the frame dimensions
     * in config.hpp (ROI_BOTTOM_LEFT_X, ROI_TOP_Y, etc.).
     *
     * @param frame     Input image (used only for dimensions).
     * @return          Binary mask (CV_8UC1), white in the ROI region.
     */
    cv::Mat get_roi_mask(const cv::Mat& frame);

    /**
     * @brief Apply a binary mask to an image via bitwise AND.
     * @param frame     Input image.
     * @param mask      Binary mask (same dimensions as frame).
     * @return          Masked image (pixels outside ROI set to zero).
     */
    cv::Mat apply_roi(const cv::Mat& frame, const cv::Mat& mask);

    /**
     * @brief Full preprocessing pipeline for the vehicle detection branch.
     *
     * Resize only — the YOLO model handles its own normalisation internally.
     *
     * @param frame     Raw BGR input frame.
     * @return          Resized BGR frame ready for YOLO inference.
     */
    cv::Mat preprocess_for_detection(const cv::Mat& frame);

    /**
     * @brief Full preprocessing pipeline for the lane detection branch.
     *
     * Resize → Grayscale → CLAHE → Gaussian Blur → Adaptive Canny → ROI mask.
     *
     * @param frame     Raw BGR input frame.
     * @return          Edge-detected, ROI-masked image ready for Hough Transform.
     */
    cv::Mat preprocess_for_lane(const cv::Mat& frame);

}
