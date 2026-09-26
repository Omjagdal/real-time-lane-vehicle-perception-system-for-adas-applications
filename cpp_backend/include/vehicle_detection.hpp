#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>

namespace vehicle_detection {

    struct Detection {
        cv::Rect bbox; // [x, y, width, height]
        int class_id;
        std::string label;
        float confidence;
        double timestamp;
        int cx;
        int cy;
        int area;
    };

    class VehicleDetector {
    public:
        VehicleDetector(const std::string& model_path = "../models/yolo11n.onnx", 
                        float conf_threshold = -1.0, 
                        float nms_threshold = -1.0);

        std::vector<Detection> detect(const cv::Mat& frame);

    private:
        cv::dnn::Net net;
        float conf_thresh;
        float nms_thresh;
        
        cv::Mat format_image(const cv::Mat& source);
    };

}
