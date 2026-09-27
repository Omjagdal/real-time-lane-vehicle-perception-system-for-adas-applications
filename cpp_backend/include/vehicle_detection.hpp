/**
 * @file    vehicle_detection.hpp
 * @brief   YOLOv11n vehicle detection module using OpenCV DNN backend.
 *
 * Wraps the ONNX-exported YOLOv11 Nano model for real-time inference.
 * The detector loads the model once at construction and provides a
 * thread-safe `detect()` method that returns filtered, NMS-processed
 * bounding boxes for vehicle-relevant COCO classes.
 *
 * Supported classes (filtered from COCO-80):
 *   - 2: car
 *   - 3: motorcycle
 *   - 5: bus
 *   - 7: truck
 *
 * @see     config.hpp for YOLO_CONF, YOLO_IOU, and VEHICLE_CLASSES.
 * @author  Om Jagdale
 */

#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>

namespace vehicle_detection {

    /**
     * @brief Single object detection result.
     *
     * Represents a detected vehicle with its spatial properties,
     * classification, and confidence score.
     */
    struct Detection {
        cv::Rect bbox;          ///< Bounding box [x, y, width, height] in pixel coordinates.
        int class_id;           ///< COCO class identifier (2=car, 3=moto, 5=bus, 7=truck).
        std::string label;      ///< Human-readable class name.
        float confidence;       ///< Detection confidence score [0.0, 1.0].
        double timestamp;       ///< Frame timestamp for temporal association.
        int cx;                 ///< Bounding box centroid X coordinate.
        int cy;                 ///< Bounding box centroid Y coordinate.
        int area;               ///< Bounding box area in pixels^2.
    };

    /**
     * @brief YOLOv11n vehicle detector with ONNX model inference.
     *
     * Loads an ONNX model file via OpenCV's DNN module and runs
     * forward inference on each input frame. Applies confidence filtering,
     * class filtering, and Non-Maximum Suppression.
     *
     * @note The model is loaded once during construction. Subsequent
     *       calls to detect() reuse the loaded network.
     */
    class VehicleDetector {
    public:
        /**
         * @brief Construct a VehicleDetector and load the ONNX model.
         * @param model_path        Path to the YOLOv11n ONNX weights file.
         * @param conf_threshold    Confidence threshold (default: config::YOLO_CONF).
         * @param nms_threshold     NMS IoU threshold (default: config::YOLO_IOU).
         * @throws cv::Exception    If the ONNX file cannot be loaded.
         */
        VehicleDetector(const std::string& model_path = "cpp_backend/models/yolo11n.onnx", 
                        float conf_threshold = -1.0, 
                        float nms_threshold = -1.0);

        /**
         * @brief Run YOLOv11n inference on a single frame.
         *
         * Pipeline: letterbox resize → blob creation → forward pass →
         * confidence filter → class filter → NMS → centroid computation.
         *
         * @param frame     Input BGR image (any resolution; internally resized to 640x640).
         * @return          Vector of Detection structs for accepted vehicle detections.
         */
        std::vector<Detection> detect(const cv::Mat& frame);

    private:
        cv::dnn::Net net;       ///< OpenCV DNN network loaded from ONNX.
        float conf_thresh;      ///< Active confidence threshold.
        float nms_thresh;       ///< Active NMS IoU threshold.

        /**
         * @brief Letterbox-resize input image to 640x640 with padding.
         * @param source    Input BGR image.
         * @return          Padded, resized image suitable for YOLO input blob.
         */
        cv::Mat format_image(const cv::Mat& source);
    };

}
