#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <fstream>
#include <chrono>
#include <uuid/uuid.h>
#include <sys/stat.h>
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include "httplib.h"

#include "config.hpp"
#include "preprocessing.hpp"
#include "lane_detection.hpp"
#include "vehicle_detection.hpp"
#include "tracker.hpp"
#include "estimators.hpp"
#include "visualization.hpp"

using json = nlohmann::json;

// --- Globals ---
std::mutex jobs_mutex;
std::map<std::string, json> jobs;
const std::string JOBS_DIR = "jobs";

std::string generate_uuid() {
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    return std::string(uuid_str);
}

void process_video(std::string job_id, std::string input_path, std::string output_path,
                   float conf, std::string device, double ego_speed, int max_frames) {
    
    {
        std::lock_guard<std::mutex> lock(jobs_mutex);
        jobs[job_id]["status"] = "processing";
    }

    try {
        cv::VideoCapture cap(input_path);
        if (!cap.isOpened()) throw std::runtime_error("Cannot open video: " + input_path);

        int total_frames = int(cap.get(cv::CAP_PROP_FRAME_COUNT));
        double fps = cap.get(cv::CAP_PROP_FPS);
        if (fps < 1.0) fps = 30.0;

        int frames_to_proc = (max_frames > 0) ? std::min(max_frames, total_frames) : total_frames;
        if (frames_to_proc <= 0) frames_to_proc = 999999;

        {
            std::lock_guard<std::mutex> lock(jobs_mutex);
            jobs[job_id]["total_frames"] = frames_to_proc;
            jobs[job_id]["fps_source"] = fps;
        }

        cv::VideoWriter writer(output_path, cv::VideoWriter::fourcc('m','p','4','v'), fps, 
                               cv::Size(config::TARGET_WIDTH, config::TARGET_HEIGHT));

        // Pipeline components
        lane_detection::LaneDetector lane_detector;
        vehicle_detection::VehicleDetector detector("../models/yolo11n.onnx", conf);
        tracker::IoUTracker trk;
        estimators::DistanceEstimator dist_est(config::TARGET_HEIGHT);
        estimators::SpeedEstimator speed_est(fps);
        estimators::FCWEngine fcw_engine(ego_speed);

        int frame_idx = 0;
        int total_brake = 0;
        int total_caution = 0;
        
        auto t_start = std::chrono::high_resolution_clock::now();

        cv::Mat frame;
        while (frame_idx < frames_to_proc && cap.read(frame)) {
            auto t0 = std::chrono::high_resolution_clock::now();

            // 1. Lane detection
            cv::Mat edge_img = preprocessing::preprocess_for_lane(frame);
            auto [left_lane, right_lane] = lane_detector.detect(edge_img, frame.rows, frame.cols);
            
            // 2. Vehicle detection
            cv::Mat prep = preprocessing::preprocess_for_detection(frame);
            auto detections = detector.detect(prep);

            // 3. Tracking
            auto active_tracks = trk.update(detections);

            // Mock tracking output for Estimators
            std::vector<estimators::TrackMock> mock_tracks;
            for (const auto& t : active_tracks) {
                mock_tracks.push_back({t.track_id, {t.bbox.x, t.bbox.y, t.bbox.x + t.bbox.width, t.bbox.y + t.bbox.height}, t.class_id, t.age});
            }

            // 4. Estimation
            auto distances = dist_est.estimate_all(mock_tracks);
            auto speeds = speed_est.update(mock_tracks, distances);
            auto fcw_res = fcw_engine.evaluate(mock_tracks, distances, speeds, ego_speed);

            // 5. Visualization
            cv::Mat out = preprocessing::resize_frame(frame);
            
            std::string critical_alert = "SAFE";
            for (const auto& pair : fcw_res) {
                if (pair.second.alert == "BRAKE") { critical_alert = "BRAKE"; total_brake++; }
                else if (pair.second.alert == "CAUTION" && critical_alert != "BRAKE") { critical_alert = "CAUTION"; total_caution++; }
            }

            auto confs = lane_detector.get_confidence();
            std::string dep_stat = lane_detector.get_departure_status();

            out = visualization::draw_lanes(out, left_lane, right_lane, dep_stat);
            out = visualization::draw_vehicles(out, active_tracks, distances, speeds, fcw_res);
            out = visualization::draw_minimap(out, active_tracks, distances, fcw_res);
            out = visualization::draw_fcw_banner(out, critical_alert);

            auto t_end = std::chrono::high_resolution_clock::now();
            double elapsed_frame = std::chrono::duration<double>(t_end - t0).count();
            double fps_live = 1.0 / std::max(elapsed_frame, 0.001);

            out = visualization::draw_hud(out, fps_live, active_tracks.size(), ego_speed, confs, dep_stat);

            writer.write(out);

            // Save preview frame
            std::string preview_path = JOBS_DIR + "/" + job_id + "/preview.jpg";
            std::vector<int> compression_params = {cv::IMWRITE_JPEG_QUALITY, 85};
            cv::imwrite(preview_path, out, compression_params);

            frame_idx++;

            // Update stats
            {
                std::lock_guard<std::mutex> lock(jobs_mutex);
                jobs[job_id]["frame"] = frame_idx;
                jobs[job_id]["fps_live"] = std::round(fps_live * 10) / 10.0;
                jobs[job_id]["num_vehicles"] = active_tracks.size();
                jobs[job_id]["brake_events"] = total_brake;
                jobs[job_id]["caution_events"] = total_caution;
            }
        }

        cap.release();
        writer.release();

        auto t_total_end = std::chrono::high_resolution_clock::now();
        double elapsed_total = std::chrono::duration<double>(t_total_end - t_start).count();

        {
            std::lock_guard<std::mutex> lock(jobs_mutex);
            jobs[job_id]["status"] = "done";
            jobs[job_id]["elapsed"] = std::round(elapsed_total * 100) / 100.0;
            jobs[job_id]["avg_fps"] = std::round((frame_idx / elapsed_total) * 10) / 10.0;
            jobs[job_id]["output_path"] = output_path;
        }

        std::cout << "Job " << job_id << " finished. Processed " << frame_idx << " frames in " << elapsed_total << "s.\n";

    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(jobs_mutex);
        jobs[job_id]["status"] = "error";
        jobs[job_id]["error"] = e.what();
        std::cerr << "Job error: " << e.what() << std::endl;
    }
}


int main() {
    std::cout << "Starting ADAS C++ Server on port 8000..." << std::endl;
    
    mkdir(JOBS_DIR.c_str(), 0777);

    httplib::Server svr;
    svr.set_payload_max_length(1024ull * 1024ull * 1024ull); // 1GB max upload


    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        json health = {
            {"status", "ok"},
            {"version", "2.0.0-cpp"},
            {"device", "cpu"},
            {"gpu_available", false},
            {"mps_available", false},
            {"python", "N/A"}
        };
        res.set_content(health.dump(), "application/json");
        res.set_header("Access-Control-Allow-Origin", "*");
    });

    svr.Post("/api/upload", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        
        if (!req.form.has_file("file")) {
            res.status = 400;
            res.set_content("{\"detail\":\"No file part\"}", "application/json");
            return;
        }

        const auto& file = req.form.get_file("file");
        
        std::string job_id = generate_uuid();
        std::string job_dir = JOBS_DIR + "/" + job_id;
        mkdir(job_dir.c_str(), 0777);

        std::string input_path = job_dir + "/input.mp4";
        std::ofstream ofs(input_path, std::ios::binary);
        ofs << file.content;
        ofs.close();

        std::string output_path = job_dir + "/annotated.mp4";

        float conf = req.has_param("conf") ? std::stof(req.get_param_value("conf")) : 0.4f;
        double ego_speed = req.has_param("ego_speed") ? std::stod(req.get_param_value("ego_speed")) : 60.0;
        int max_frames = req.has_param("max_frames") ? std::stoi(req.get_param_value("max_frames")) : 0;
        std::string device = req.has_param("device") ? req.get_param_value("device") : "cpu";

        {
            std::lock_guard<std::mutex> lock(jobs_mutex);
            jobs[job_id] = {
                {"job_id", job_id},
                {"filename", file.filename},
                {"status", "queued"},
                {"frame", 0},
                {"total_frames", 0},
                {"fps_live", 0.0},
                {"num_vehicles", 0},
                {"brake_events", 0},
                {"caution_events", 0},
                {"conf", conf},
                {"ego_speed", ego_speed},
                {"device", device},
                {"file_size_mb", std::round((file.content.length() / 1e6) * 10) / 10.0}
            };
        }

        std::thread t(process_video, job_id, input_path, output_path, conf, device, ego_speed, max_frames);
        t.detach();

        json response = {{"job_id", job_id}};
        res.set_content(response.dump(), "application/json");
    });

    svr.Get(R"(/api/jobs/(.*))", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::string job_id = req.matches[1];
        
        std::lock_guard<std::mutex> lock(jobs_mutex);
        if (jobs.find(job_id) == jobs.end()) {
            res.status = 404;
            return;
        }
        res.set_content(jobs[job_id].dump(), "application/json");
    });

    svr.Get(R"(/api/process/(.*))", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::string job_id = req.matches[1];
        
        res.set_chunked_content_provider("text/event-stream",
            [job_id](size_t offset, httplib::DataSink &sink) {
                while (true) {
                    std::string payload;
                    std::string status;
                    {
                        std::lock_guard<std::mutex> lock(jobs_mutex);
                        if (jobs.find(job_id) != jobs.end()) {
                            payload = jobs[job_id].dump();
                            status = jobs[job_id]["status"];
                        } else {
                            return false;
                        }
                    }
                    
                    std::string msg = "data: " + payload + "\n\n";
                    if (!sink.write(msg.data(), msg.size())) return false;
                    
                    if (status == "done" || status == "error") {
                        sink.done();
                        return true;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(250));
                }
                return true;
            }
        );
    });

    svr.Get(R"(/api/frame/(.*))", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::string job_id = req.matches[1];
        std::string preview_path = JOBS_DIR + "/" + job_id + "/preview.jpg";
        
        std::ifstream ifs(preview_path, std::ios::binary);
        if (ifs) {
            std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            res.set_content(content, "image/jpeg");
        } else {
            res.status = 404;
        }
    });

    svr.Get(R"(/api/download/(.*))", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::string job_id = req.matches[1];
        std::string output_path = JOBS_DIR + "/" + job_id + "/annotated.mp4";
        
        std::ifstream ifs(output_path, std::ios::binary);
        if (ifs) {
            std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            res.set_content(content, "video/mp4");
        } else {
            res.status = 404;
        }
    });

    svr.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS, DELETE");
        res.set_header("Access-Control-Allow-Headers", "*");
        res.status = 200;
    });

    svr.listen("0.0.0.0", 8000);
    return 0;
}
