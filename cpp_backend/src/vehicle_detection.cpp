#include "vehicle_detection.hpp"
#include "config.hpp"
#include <iostream>
#include <chrono>

namespace vehicle_detection {

    VehicleDetector::VehicleDetector(const std::string& model_path, float conf_threshold, float nms_threshold) {
        conf_thresh = (conf_threshold > 0) ? conf_threshold : config::YOLO_CONF;
        nms_thresh = (nms_threshold > 0) ? nms_threshold : config::YOLO_IOU;

        try {
            net = cv::dnn::readNetFromONNX(model_path);
            std::cout << "Loaded ONNX model: " << model_path << std::endl;
        } catch (const cv::Exception& e) {
            std::cerr << "Error loading model: " << e.what() << std::endl;
        }
    }

    cv::Mat VehicleDetector::format_image(const cv::Mat& source) {
        // YOLO expects 640x640 input typically, but since we export it without dynamic axes 
        // or specifically 640x640, we must resize.
        // Assuming the exported ONNX expects 640x640 (ultralytics default)
        int col = source.cols;
        int row = source.rows;
        int _max = std::max(col, row);
        cv::Mat result = cv::Mat::zeros(_max, _max, CV_8UC3);
        source.copyTo(result(cv::Rect(0, 0, col, row)));
        return result;
    }

    std::vector<Detection> VehicleDetector::detect(const cv::Mat& frame) {
        auto t_start = std::chrono::high_resolution_clock::now();

        cv::Mat input_image = format_image(frame);
        cv::Mat blob = cv::dnn::blobFromImage(input_image, 1.0 / 255.0, cv::Size(640, 640), cv::Scalar(), true, false);

        net.setInput(blob);
        std::vector<cv::Mat> outputs;
        net.forward(outputs, net.getUnconnectedOutLayersNames());

        float x_factor = static_cast<float>(input_image.cols) / 640.0f;
        float y_factor = static_cast<float>(input_image.rows) / 640.0f;

        // Output shape is typically 1 x 84 x 8400 (YOLOv8/11)
        float* data = (float*)outputs[0].data;
        const int dimensions = outputs[0].size[1]; // 84
        const int rows = outputs[0].size[2];       // 8400

        cv::Mat output(dimensions, rows, CV_32F, data);
        output = output.t(); // Transpose to rows x 84

        std::vector<int> class_ids;
        std::vector<float> confidences;
        std::vector<cv::Rect> boxes;

        for (int i = 0; i < rows; ++i) {
            float* row_ptr = output.ptr<float>(i);
            float* classes_scores = row_ptr + 4;
            
            cv::Mat scores(1, dimensions - 4, CV_32F, classes_scores);
            cv::Point class_id;
            double max_class_score;
            cv::minMaxLoc(scores, 0, &max_class_score, 0, &class_id);

            if (max_class_score > conf_thresh) {
                // Check if class is in our vehicle/pedestrian whitelist
                if (config::VEHICLE_CLASSES.find(class_id.x) != config::VEHICLE_CLASSES.end()) {
                    
                    float x = row_ptr[0];
                    float y = row_ptr[1];
                    float w = row_ptr[2];
                    float h = row_ptr[3];

                    int left = int((x - 0.5 * w) * x_factor);
                    int top = int((y - 0.5 * h) * y_factor);
                    int width = int(w * x_factor);
                    int height = int(h * y_factor);

                    int bbox_area = width * height;
                    int cy = top + height / 2;
                    
                    if (cy < frame.rows * config::DETECTION_ROI_TOP && bbox_area < config::MIN_DETECTION_AREA * 3) {
                        continue;
                    }
                    if (bbox_area < config::MIN_DETECTION_AREA) {
                        continue;
                    }

                    class_ids.push_back(class_id.x);
                    confidences.push_back((float)max_class_score);
                    boxes.push_back(cv::Rect(left, top, width, height));
                }
            }
        }

        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, confidences, conf_thresh, nms_thresh, indices);

        auto t_end = std::chrono::high_resolution_clock::now();
        double timestamp = std::chrono::duration<double>(t_end.time_since_epoch()).count();

        std::vector<Detection> results;
        for (int idx : indices) {
            Detection d;
            d.bbox = boxes[idx];
            d.class_id = class_ids[idx];
            d.label = config::VEHICLE_CLASSES.at(d.class_id);
            d.confidence = confidences[idx];
            d.timestamp = timestamp;
            d.cx = d.bbox.x + d.bbox.width / 2;
            d.cy = d.bbox.y + d.bbox.height / 2;
            d.area = d.bbox.width * d.bbox.height;
            results.push_back(d);
        }

        return results;
    }

}
