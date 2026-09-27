/**
 * @file    main.cpp
 * @brief   HTTP server and pipeline orchestration for the ADAS C++ backend.
 *
 * Implements a REST + SSE API server using cpp-httplib that receives video
 * uploads, runs the full perception pipeline in a background thread, and
 * streams real-time progress to the React frontend.
 *
 * Endpoints:
 *   GET  /api/health          — Server health and capability report
 *   POST /api/upload          — Upload video and start processing
 *   GET  /api/process/:id     — SSE stream of processing progress
 *   GET  /api/frame/:id       — Latest annotated preview frame (JPEG)
 *   GET  /api/jobs/:id        — Job metadata and results
 *   GET  /api/download/:id    — Download annotated output video
 *
 * @author  Om Jagdale
 */

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <map>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <uuid/uuid.h>
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
namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────
//  Logging Utility
// ─────────────────────────────────────────────────────────────────────

namespace log {
    enum class Level { DEBUG, INFO, WARN, ERROR };

    inline const char* level_str(Level lvl) {
        switch (lvl) {
            case Level::DEBUG: return "DEBUG";
            case Level::INFO:  return "INFO ";
            case Level::WARN:  return "WARN ";
            case Level::ERROR: return "ERROR";
        }
        return "?????";
    }

    inline std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        char buf[32];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&time));
        char result[40];
        std::snprintf(result, sizeof(result), "%s.%03d", buf, static_cast<int>(ms.count()));
        return std::string(result);
    }

    template<typename... Args>
    void msg(Level lvl, const std::string& component, const std::string& message) {
        std::cout << "[" << timestamp() << "] [" << level_str(lvl) << "] "
                  << "[" << component << "] " << message << "\n";
    }

    inline void info(const std::string& c, const std::string& m)  { msg(Level::INFO, c, m); }
    inline void warn(const std::string& c, const std::string& m)  { msg(Level::WARN, c, m); }
    inline void error(const std::string& c, const std::string& m) { msg(Level::ERROR, c, m); }
    inline void debug(const std::string& c, const std::string& m) { msg(Level::DEBUG, c, m); }
}

// ─────────────────────────────────────────────────────────────────────
//  Thread-Safe Job Store
// ─────────────────────────────────────────────────────────────────────

class JobStore {
public:
    void create(const std::string& id, json initial_state) {
        std::unique_lock lock(mtx_);
        store_[id] = std::move(initial_state);
    }

    void update(const std::string& id, const std::function<void(json&)>& mutator) {
        std::unique_lock lock(mtx_);
        if (auto it = store_.find(id); it != store_.end()) {
            mutator(it->second);
        }
    }

    json get(const std::string& id) const {
        std::shared_lock lock(mtx_);
        if (auto it = store_.find(id); it != store_.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool exists(const std::string& id) const {
        std::shared_lock lock(mtx_);
        return store_.count(id) > 0;
    }

private:
    mutable std::shared_mutex mtx_;
    std::map<std::string, json> store_;
};

// ─────────────────────────────────────────────────────────────────────
//  Globals
// ─────────────────────────────────────────────────────────────────────

static JobStore g_jobs;
static const fs::path JOBS_DIR = "jobs";

static std::string generate_uuid() {
    uuid_t uuid;
    uuid_generate(uuid);
    char buf[37];
    uuid_unparse_lower(uuid, buf);
    return std::string(buf);
}

// ─────────────────────────────────────────────────────────────────────
//  CORS Middleware
// ─────────────────────────────────────────────────────────────────────

static void apply_cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// ─────────────────────────────────────────────────────────────────────
//  Pipeline Runner
// ─────────────────────────────────────────────────────────────────────

/**
 * @brief   Runs the full ADAS perception pipeline on a video file.
 *
 * Executes in a detached background thread. Streams progress updates
 * to the job store, which the SSE endpoint reads from.
 *
 * Pipeline stages per frame:
 *   1. Preprocessing (resize, edge detection, ROI)
 *   2. Lane detection (Hough Transform + temporal smoothing)
 *   3. Vehicle detection (YOLOv11n ONNX inference)
 *   4. Multi-object tracking (IoU + Kalman)
 *   5. Distance estimation (pinhole camera model)
 *   6. Speed estimation (multi-frame displacement)
 *   7. FCW evaluation (TTC + hysteresis)
 *   8. Visualization (overlay, HUD, minimap)
 */
static void run_pipeline(
    const std::string& job_id,
    const fs::path& input_path,
    const fs::path& output_path,
    float conf,
    double ego_speed,
    int max_frames
) {
    const std::string TAG = "Pipeline";

    g_jobs.update(job_id, [](json& j) { j["status"] = "processing"; });
    log::info(TAG, "Starting job " + job_id);

    try {
        // ── Video I/O ────────────────────────────────────────────────
        cv::VideoCapture cap(input_path.string());
        if (!cap.isOpened()) {
            throw std::runtime_error("Cannot open video: " + input_path.string());
        }

        const auto total_frames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
        double fps = cap.get(cv::CAP_PROP_FPS);
        if (fps < 1.0) fps = 30.0;

        const int frames_to_process = (max_frames > 0)
            ? std::min(max_frames, total_frames)
            : (total_frames > 0 ? total_frames : 999999);

        g_jobs.update(job_id, [&](json& j) {
            j["total_frames"] = frames_to_process;
            j["fps_source"] = fps;
        });

        cv::VideoWriter writer(
            output_path.string(),
            cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
            fps,
            cv::Size(config::TARGET_WIDTH, config::TARGET_HEIGHT)
        );

        if (!writer.isOpened()) {
            throw std::runtime_error("Cannot open video writer: " + output_path.string());
        }

        // ── Perception Modules (RAII construction) ───────────────────
        lane_detection::LaneDetector  lane_detector;
        vehicle_detection::VehicleDetector detector("cpp_backend/models/yolo11n.onnx", conf);
        tracker::IoUTracker           trk;
        estimators::DistanceEstimator dist_est(config::TARGET_HEIGHT);
        estimators::SpeedEstimator    speed_est(fps);
        estimators::FCWEngine         fcw_engine(ego_speed);

        log::info(TAG, "Pipeline initialised: " + std::to_string(frames_to_process) +
                       " frames @ " + std::to_string(static_cast<int>(fps)) + " FPS");

        // ── Frame Loop ───────────────────────────────────────────────
        int frame_idx = 0;
        int total_brake = 0;
        int total_caution = 0;
        const auto t_start = std::chrono::steady_clock::now();

        const fs::path preview_path = JOBS_DIR / job_id / "preview.jpg";
        const std::vector<int> jpeg_params = {cv::IMWRITE_JPEG_QUALITY, 85};

        cv::Mat frame;
        while (frame_idx < frames_to_process && cap.read(frame)) {
            const auto t_frame_start = std::chrono::steady_clock::now();

            // Stage 1: Preprocessing
            cv::Mat resized = preprocessing::resize_frame(frame);

            // Stage 2: Lane Detection
            cv::Mat edge_img = preprocessing::preprocess_for_lane(frame);
            auto [left_lane, right_lane] = lane_detector.detect(
                edge_img, resized.rows, resized.cols
            );

            // Stage 3: Vehicle Detection
            cv::Mat det_input = preprocessing::preprocess_for_detection(frame);
            auto detections = detector.detect(det_input);

            // Stage 4: Multi-Object Tracking
            auto active_tracks = trk.update(detections);

            // Stage 5–6: Build estimator input (decouple from tracker struct)
            std::vector<estimators::TrackMock> estimation_input;
            estimation_input.reserve(active_tracks.size());
            for (const auto& t : active_tracks) {
                estimation_input.push_back({
                    t.track_id,
                    {t.bbox.x, t.bbox.y, t.bbox.x + t.bbox.width, t.bbox.y + t.bbox.height},
                    t.class_id,
                    t.age
                });
            }

            auto distances = dist_est.estimate_all(estimation_input);
            auto speeds    = speed_est.update(estimation_input, distances);

            // Stage 7: Forward Collision Warning
            auto fcw_results = fcw_engine.evaluate(
                estimation_input, distances, speeds, ego_speed
            );

            // Determine frame-level critical alert
            std::string critical_alert = "SAFE";
            for (const auto& [id, result] : fcw_results) {
                if (result.alert == "BRAKE") {
                    critical_alert = "BRAKE";
                    ++total_brake;
                } else if (result.alert == "CAUTION" && critical_alert != "BRAKE") {
                    critical_alert = "CAUTION";
                    ++total_caution;
                }
            }

            // Stage 8: Visualization
            auto [left_conf, right_conf] = lane_detector.get_confidence();
            const auto departure_status = lane_detector.get_departure_status();

            cv::Mat out = resized.clone();
            visualization::draw_lanes(out, left_lane, right_lane, departure_status);
            visualization::draw_vehicles(out, active_tracks, distances, speeds, fcw_results);
            visualization::draw_minimap(out, active_tracks, distances, fcw_results);
            visualization::draw_fcw_banner(out, critical_alert);

            const auto t_frame_end = std::chrono::steady_clock::now();
            const double frame_ms = std::chrono::duration<double, std::milli>(
                t_frame_end - t_frame_start).count();
            const double fps_live = 1000.0 / std::max(frame_ms, 0.1);

            visualization::draw_hud(out, fps_live, static_cast<int>(active_tracks.size()),
                                    ego_speed, {left_conf, right_conf}, departure_status);

            writer.write(out);
            cv::imwrite(preview_path.string(), out, jpeg_params);

            ++frame_idx;

            // Update job store (throttled to reduce lock contention)
            if (frame_idx % 2 == 0 || frame_idx == frames_to_process) {
                g_jobs.update(job_id, [&](json& j) {
                    j["frame"]          = frame_idx;
                    j["fps_live"]       = std::round(fps_live * 10.0) / 10.0;
                    j["num_vehicles"]   = static_cast<int>(active_tracks.size());
                    j["brake_events"]   = total_brake;
                    j["caution_events"] = total_caution;
                });
            }
        }

        // ── Finalise ─────────────────────────────────────────────────
        cap.release();
        writer.release();

        const auto t_end = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(t_end - t_start).count();
        const double avg_fps = (elapsed > 0) ? (frame_idx / elapsed) : 0.0;

        g_jobs.update(job_id, [&](json& j) {
            j["status"]      = "done";
            j["frame"]       = frame_idx;
            j["elapsed"]     = std::round(elapsed * 100.0) / 100.0;
            j["avg_fps"]     = std::round(avg_fps * 10.0) / 10.0;
            j["output_path"] = output_path.string();
        });

        log::info(TAG, "Job " + job_id + " completed: " +
                       std::to_string(frame_idx) + " frames in " +
                       std::to_string(static_cast<int>(elapsed)) + "s (" +
                       std::to_string(static_cast<int>(avg_fps)) + " FPS)");

    } catch (const std::exception& e) {
        g_jobs.update(job_id, [&](json& j) {
            j["status"] = "error";
            j["error"]  = e.what();
        });
        log::error(TAG, "Job " + job_id + " failed: " + std::string(e.what()));
    }
}

// ─────────────────────────────────────────────────────────────────────
//  HTTP Server
// ─────────────────────────────────────────────────────────────────────

int main() {
    const std::string TAG = "Server";

    log::info(TAG, "═══════════════════════════════════════════════");
    log::info(TAG, "  ADAS Perception Pipeline — C++17 Backend");
    log::info(TAG, "  Version    : 2.0.0");
    log::info(TAG, "  OpenCV     : " + std::string(CV_VERSION));
    log::info(TAG, "  Endpoint   : http://0.0.0.0:8000");
    log::info(TAG, "═══════════════════════════════════════════════");

    fs::create_directories(JOBS_DIR);

    httplib::Server svr;
    svr.set_payload_max_length(1024ULL * 1024ULL * 1024ULL);  // 1 GB max upload

    // ── Health Check ─────────────────────────────────────────────────
    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        apply_cors(res);
        json health = {
            {"status", "ok"},
            {"version", "2.0.0-cpp17"},
            {"backend", "C++17 / OpenCV DNN"},
            {"opencv_version", CV_VERSION},
            {"device", config::DEVICE},
            {"gpu_available", false},
            {"mps_available", false}
        };
        res.set_content(health.dump(), "application/json");
    });

    // ── Upload + Start Processing ────────────────────────────────────
    svr.Post("/api/upload", [](const httplib::Request& req, httplib::Response& res) {
        apply_cors(res);

        if (!req.form.has_file("file")) {
            res.status = 400;
            res.set_content(R"({"detail":"No file part"})", "application/json");
            return;
        }

        const auto& file = req.form.get_file("file");
        const auto job_id = generate_uuid();
        const auto job_dir = JOBS_DIR / job_id;
        fs::create_directories(job_dir);

        // Persist uploaded video
        const auto input_path  = job_dir / "input.mp4";
        const auto output_path = job_dir / "annotated.mp4";

        {
            std::ofstream ofs(input_path, std::ios::binary);
            ofs.write(file.content.data(), static_cast<std::streamsize>(file.content.size()));
        }

        // Parse pipeline parameters
        const float  conf       = req.has_param("conf")       ? std::stof(req.get_param_value("conf"))       : 0.4f;
        const double ego_speed  = req.has_param("ego_speed")   ? std::stod(req.get_param_value("ego_speed"))   : 60.0;
        const int    max_frames = req.has_param("max_frames")  ? std::stoi(req.get_param_value("max_frames"))  : 0;
        const auto   device     = req.has_param("device")      ? req.get_param_value("device")                 : "cpu";

        // Initialise job state
        g_jobs.create(job_id, {
            {"job_id",          job_id},
            {"filename",        file.filename},
            {"status",          "queued"},
            {"frame",           0},
            {"total_frames",    0},
            {"fps_live",        0.0},
            {"fps_source",      0.0},
            {"num_vehicles",    0},
            {"brake_events",    0},
            {"caution_events",  0},
            {"conf",            conf},
            {"ego_speed",       ego_speed},
            {"device",          device},
            {"file_size_mb",    std::round((file.content.size() / 1e6) * 10.0) / 10.0}
        });

        log::info("Upload", "Job " + job_id + " created: " +
                  file.filename + " (" + std::to_string(file.content.size() / 1048576) + " MB)");

        // Launch pipeline in detached thread
        std::thread(run_pipeline, job_id, input_path, output_path,
                    conf, ego_speed, max_frames).detach();

        res.set_content(json{{"job_id", job_id}}.dump(), "application/json");
    });

    // ── Job Status ───────────────────────────────────────────────────
    svr.Get(R"(/api/jobs/(.*))", [](const httplib::Request& req, httplib::Response& res) {
        apply_cors(res);
        const auto job_id = std::string(req.matches[1]);
        auto data = g_jobs.get(job_id);
        if (data.is_null()) {
            res.status = 404;
            return;
        }
        res.set_content(data.dump(), "application/json");
    });

    // ── SSE Progress Stream ──────────────────────────────────────────
    svr.Get(R"(/api/process/(.*))", [](const httplib::Request& req, httplib::Response& res) {
        apply_cors(res);
        const auto job_id = std::string(req.matches[1]);

        res.set_chunked_content_provider("text/event-stream",
            [job_id](size_t /*offset*/, httplib::DataSink& sink) {
                while (true) {
                    auto data = g_jobs.get(job_id);
                    if (data.is_null()) return false;

                    const auto payload = "data: " + data.dump() + "\n\n";
                    if (!sink.write(payload.data(), payload.size())) return false;

                    const auto status = data.value("status", "");
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

    // ── Live Preview Frame ───────────────────────────────────────────
    svr.Get(R"(/api/frame/(.*))", [](const httplib::Request& req, httplib::Response& res) {
        apply_cors(res);
        const auto job_id = std::string(req.matches[1]);
        const auto preview_path = JOBS_DIR / job_id / "preview.jpg";

        std::ifstream ifs(preview_path, std::ios::binary);
        if (!ifs) {
            res.status = 404;
            return;
        }
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        res.set_content(std::move(content), "image/jpeg");
    });

    // ── Download Annotated Video ─────────────────────────────────────
    svr.Get(R"(/api/download/(.*))", [](const httplib::Request& req, httplib::Response& res) {
        apply_cors(res);
        const auto job_id = std::string(req.matches[1]);
        const auto output_path = JOBS_DIR / job_id / "annotated.mp4";

        std::ifstream ifs(output_path, std::ios::binary);
        if (!ifs) {
            res.status = 404;
            return;
        }
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        res.set_header("Content-Disposition",
                       "attachment; filename=\"adas_" + job_id.substr(0, 8) + ".mp4\"");
        res.set_content(std::move(content), "video/mp4");
    });

    // ── CORS Preflight ───────────────────────────────────────────────
    svr.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        apply_cors(res);
        res.status = 204;  // No Content (correct preflight response)
    });

    log::info(TAG, "Listening on http://0.0.0.0:8000");
    svr.listen("0.0.0.0", 8000);

    return 0;
}
