#pragma once

#include <vector>
#include <map>
#include <string>

namespace estimators {

    struct TrackMock {
        int track_id;
        std::vector<int> bbox;
        int class_id;
        int age;
    };

    struct FCWResult {
        std::string alert;
        double ttc;
        double current_dist;
        double current_speed;
        double req_decel;
    };

    class DistanceEstimator {
    public:
        DistanceEstimator(int frame_height);
        std::map<int, double> estimate_all(const std::vector<TrackMock>& tracks);
    private:
        int frame_height;
        std::map<int, double> history;
        double get_real_width(int class_id) const;
    };

    class SpeedEstimator {
    public:
        SpeedEstimator(double fps);
        std::map<int, double> update(const std::vector<TrackMock>& tracks, const std::map<int, double>& distances);
    private:
        double fps;
        double dt;
        std::map<int, std::vector<double>> dist_history;
        std::map<int, double> speed_history;
    };

    class FCWEngine {
    public:
        FCWEngine(double ego_speed_kmh);
        std::map<int, FCWResult> evaluate(const std::vector<TrackMock>& tracks, 
                                          const std::map<int, double>& distances,
                                          const std::map<int, double>& speeds,
                                          double ego_speed_kmh = -1.0);
    private:
        double ego_speed;
        std::map<int, int> alert_counters;
    };

}
