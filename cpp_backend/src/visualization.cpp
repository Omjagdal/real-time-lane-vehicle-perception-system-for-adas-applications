#include "visualization.hpp"
#include "config.hpp"
#include <iomanip>
#include <sstream>

namespace visualization {

    static std::string format_float(double val, int precision = 1) {
        std::ostringstream out;
        out << std::fixed << std::setprecision(precision) << val;
        return out.str();
    }

    cv::Mat draw_lanes(cv::Mat& frame, 
                       const lane_detection::Lines& left_line, 
                       const lane_detection::Lines& right_line, 
                       const std::string& departure_status) {
        
        cv::Scalar colour(50, 205, 50); // Green (BGR: 50, 205, 50 -> mostly green)
        if (departure_status == "left" || departure_status == "right") {
            colour = cv::Scalar(0, 165, 255); // Orange in BGR
        }

        if (left_line.has_value() && right_line.has_value()) {
            auto l = left_line.value()[0];
            auto r = right_line.value()[0];

            cv::Point pts[1][4];
            pts[0][0] = cv::Point(l[0], l[1]);
            pts[0][1] = cv::Point(l[2], l[3]);
            pts[0][2] = cv::Point(r[2], r[3]);
            pts[0][3] = cv::Point(r[0], r[1]);
            
            const cv::Point* ppt[1] = { pts[0] };
            int npt[] = { 4 };
            
            cv::Mat overlay = frame.clone();
            cv::fillPoly(overlay, ppt, npt, 1, colour);
            cv::addWeighted(overlay, 0.25, frame, 0.75, 0, frame);
        }

        if (left_line.has_value()) {
            auto l = left_line.value()[0];
            cv::line(frame, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), colour, 3);
        }
        
        if (right_line.has_value()) {
            auto r = right_line.value()[0];
            cv::line(frame, cv::Point(r[0], r[1]), cv::Point(r[2], r[3]), colour, 3);
        }

        return frame;
    }

    cv::Mat draw_vehicles(cv::Mat& frame, 
                          const std::vector<tracker::Track>& tracks, 
                          const std::map<int, double>& distances, 
                          const std::map<int, double>& speeds, 
                          const std::map<int, estimators::FCWResult>& fcw_results) {
        
        for (const auto& trk : tracks) {
            cv::Scalar box_colour(255, 191, 0); // Deep Sky Blue / Gold
            std::string alert = "SAFE";

            if (fcw_results.find(trk.track_id) != fcw_results.end()) {
                alert = fcw_results.at(trk.track_id).alert;
                if (alert == "BRAKE") box_colour = cv::Scalar(0, 0, 255);
                else if (alert == "CAUTION") box_colour = cv::Scalar(0, 165, 255);
            }

            cv::rectangle(frame, trk.bbox, box_colour, 2);

            std::string header_txt = "#" + std::to_string(trk.track_id) + " " + trk.label;
            
            int b_w = 0, b_h = 0;
            cv::Size text_size = cv::getTextSize(header_txt, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &b_h);
            b_w = text_size.width;

            cv::Rect header_bg(trk.bbox.x, trk.bbox.y - 20, b_w + 10, 20);
            cv::rectangle(frame, header_bg, cv::Scalar(20, 20, 20), cv::FILLED);
            cv::putText(frame, header_txt, cv::Point(trk.bbox.x + 5, trk.bbox.y - 5), 
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, box_colour, 1, cv::LINE_AA);

            if (distances.find(trk.track_id) != distances.end() && speeds.find(trk.track_id) != speeds.end()) {
                double dist = distances.at(trk.track_id);
                double speed = speeds.at(trk.track_id);
                double ttc = 99.9;
                
                if (fcw_results.find(trk.track_id) != fcw_results.end()) {
                    ttc = fcw_results.at(trk.track_id).ttc;
                }

                std::string meta_txt = format_float(dist, 1) + "m " + 
                                       format_float(speed, 0) + "km/h TTC:" + 
                                       format_float(ttc, 1) + "s";

                cv::Size meta_size = cv::getTextSize(meta_txt, cv::FONT_HERSHEY_SIMPLEX, 0.45, 1, &b_h);
                cv::Rect meta_bg(trk.bbox.x, trk.bbox.y + trk.bbox.height, meta_size.width + 10, 18);
                
                cv::Mat overlay = frame.clone();
                cv::rectangle(overlay, meta_bg, cv::Scalar(10, 10, 10), cv::FILLED);
                cv::addWeighted(overlay, 0.7, frame, 0.3, 0, frame);
                
                cv::putText(frame, meta_txt, cv::Point(trk.bbox.x + 5, trk.bbox.y + trk.bbox.height + 13), 
                            cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(220, 220, 220), 1, cv::LINE_AA);
            }
        }
        return frame;
    }

    cv::Mat draw_fcw_banner(cv::Mat& frame, const std::string& alert_level) {
        if (alert_level == "SAFE") return frame;

        cv::Scalar colour;
        std::string text;

        if (alert_level == "BRAKE") {
            colour = cv::Scalar(0, 0, 255);
            text = "!!! COLLISION WARNING - BRAKE !!!";
        } else {
            colour = cv::Scalar(0, 165, 255);
            text = "CAUTION - VEHICLE AHEAD";
        }

        cv::Mat overlay = frame.clone();
        cv::rectangle(overlay, cv::Rect(0, 0, frame.cols, 80), colour, cv::FILLED);
        cv::addWeighted(overlay, 0.25, frame, 0.75, 0, frame);
        cv::line(frame, cv::Point(0, 80), cv::Point(frame.cols, 80), colour, 3);

        int baseline;
        cv::Size size = cv::getTextSize(text, cv::FONT_HERSHEY_DUPLEX, 1.2, 2, &baseline);
        cv::putText(frame, text, cv::Point((frame.cols - size.width) / 2, 50), 
                    cv::FONT_HERSHEY_DUPLEX, 1.2, colour, 2, cv::LINE_AA);
        
        return frame;
    }

    cv::Mat draw_minimap(cv::Mat& frame, 
                         const std::vector<tracker::Track>& tracks, 
                         const std::map<int, double>& distances, 
                         const std::map<int, estimators::FCWResult>& fcw_results) {
        
        if (!config::SHOW_MINIMAP) return frame;

        int m_size = config::MINIMAP_SIZE;
        int p_x = frame.cols - m_size - 20;
        int p_y = frame.rows - m_size - 20;

        cv::Mat overlay = frame.clone();
        cv::rectangle(overlay, cv::Rect(p_x, p_y, m_size, m_size), cv::Scalar(15, 10, 10), cv::FILLED);
        cv::addWeighted(overlay, 0.8, frame, 0.2, 0, frame);
        cv::rectangle(frame, cv::Rect(p_x, p_y, m_size, m_size), cv::Scalar(60, 60, 60), 1);

        // Grid
        cv::line(frame, cv::Point(p_x, p_y + m_size / 2), cv::Point(p_x + m_size, p_y + m_size / 2), cv::Scalar(50, 50, 50), 1);
        cv::line(frame, cv::Point(p_x, p_y + m_size / 4), cv::Point(p_x + m_size, p_y + m_size / 4), cv::Scalar(50, 50, 50), 1);
        cv::line(frame, cv::Point(p_x, p_y + 3 * m_size / 4), cv::Point(p_x + m_size, p_y + 3 * m_size / 4), cv::Scalar(50, 50, 50), 1);

        cv::putText(frame, "60m", cv::Point(p_x + 2, p_y + m_size / 4 - 3), cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(150, 150, 150), 1);
        cv::putText(frame, "40m", cv::Point(p_x + 2, p_y + m_size / 2 - 3), cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(150, 150, 150), 1);
        cv::putText(frame, "20m", cv::Point(p_x + 2, p_y + 3 * m_size / 4 - 3), cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(150, 150, 150), 1);

        // Ego vehicle
        int ego_x = p_x + m_size / 2;
        int ego_y = p_y + m_size - 10;
        cv::rectangle(frame, cv::Rect(ego_x - 6, ego_y - 12, 12, 24), cv::Scalar(0, 200, 255), cv::FILLED);

        // Map range max
        double max_dist = 80.0;

        for (const auto& trk : tracks) {
            if (distances.find(trk.track_id) == distances.end()) continue;
            
            double d = distances.at(trk.track_id);
            if (d > max_dist) continue;

            double cx = trk.bbox.x + trk.bbox.width / 2.0;
            double norm_x = (cx - frame.cols / 2.0) / (frame.cols / 2.0);
            
            int map_x = ego_x + int(norm_x * m_size / 2.5);
            int map_y = ego_y - int((d / max_dist) * m_size);

            map_x = std::clamp(map_x, p_x + 5, p_x + m_size - 5);
            map_y = std::clamp(map_y, p_y + 5, p_y + m_size - 5);

            cv::Scalar dot_col(200, 200, 200);
            if (fcw_results.find(trk.track_id) != fcw_results.end()) {
                std::string alert = fcw_results.at(trk.track_id).alert;
                if (alert == "BRAKE") dot_col = cv::Scalar(0, 0, 255);
                else if (alert == "CAUTION") dot_col = cv::Scalar(0, 165, 255);
            }

            cv::circle(frame, cv::Point(map_x, map_y), 4, dot_col, cv::FILLED);
            cv::putText(frame, format_float(d, 0) + "m", cv::Point(map_x + 6, map_y + 3), 
                        cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(200, 200, 200), 1);
        }

        return frame;
    }

    cv::Mat draw_hud(cv::Mat& frame, 
                     double fps, 
                     int num_tracks, 
                     double ego_speed_kmh, 
                     std::tuple<float, float> lane_confidence, 
                     const std::string& departure_status) {
        
        cv::Mat overlay = frame.clone();
        cv::rectangle(overlay, cv::Rect(20, 20, 250, 140), cv::Scalar(15, 10, 10), cv::FILLED);
        cv::addWeighted(overlay, config::HUD_OPACITY, frame, 1.0 - config::HUD_OPACITY, 0, frame);
        cv::rectangle(frame, cv::Rect(20, 20, 250, 140), cv::Scalar(255, 191, 0), 1);

        cv::putText(frame, "ADAS SYSTEM v2.0-CPP", cv::Point(35, 45), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 191, 0), 2);
        cv::line(frame, cv::Point(20, 55), cv::Point(270, 55), cv::Scalar(255, 191, 0), 1);

        cv::putText(frame, "FPS    : " + format_float(fps, 1), cv::Point(35, 75), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(220, 220, 220), 1);
        cv::putText(frame, "TARGETS: " + std::to_string(num_tracks), cv::Point(35, 95), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(220, 220, 220), 1);
        cv::putText(frame, "SPEED  : " + format_float(ego_speed_kmh, 1) + " km/h", cv::Point(35, 115), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(220, 220, 220), 1);
        
        std::string dep_txt = (departure_status == "left") ? "LEFT WARNING" : (departure_status == "right") ? "RIGHT WARNING" : "CENTRED";
        cv::Scalar dep_col = (departure_status == "centred") ? cv::Scalar(50, 205, 50) : cv::Scalar(0, 165, 255);
        cv::putText(frame, "LANE   : " + dep_txt, cv::Point(35, 135), cv::FONT_HERSHEY_SIMPLEX, 0.45, dep_col, 1);

        return frame;
    }

}
