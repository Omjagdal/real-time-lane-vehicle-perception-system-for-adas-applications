# Real-Time Lane and Vehicle Perception System for ADAS Applications

A modular, real-time **Advanced Driver Assistance System (ADAS)** perception pipeline implementing lane detection, vehicle detection and tracking, monocular distance and speed estimation, and forward collision warning. Designed as a production-grade reference architecture for automotive computer vision, with dual C++17 and Python backends, a reactive web dashboard, and ISO 15622-aligned collision warning logic.

![C++](https://img.shields.io/badge/C++-17-00599C.svg?logo=cplusplus)
![Python](https://img.shields.io/badge/Python-3.11+-3776AB.svg?logo=python&logoColor=white)
![OpenCV](https://img.shields.io/badge/OpenCV-4.8+-5C3EE8.svg?logo=opencv)
![ONNX](https://img.shields.io/badge/ONNX-Runtime-005CED.svg?logo=onnx)
![YOLOv11](https://img.shields.io/badge/YOLOv11n-Ultralytics-111F68.svg)
![React](https://img.shields.io/badge/React-18+-61DAFB.svg?logo=react&logoColor=black)
![FastAPI](https://img.shields.io/badge/FastAPI-0.135+-009688.svg?logo=fastapi)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

---

## Table of Contents

1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [Perception Pipeline](#perception-pipeline)
4. [Module Specifications](#module-specifications)
5. [Project Structure](#project-structure)
6. [Build and Installation](#build-and-installation)
7. [Usage](#usage)
8. [Performance Benchmarks](#performance-benchmarks)
9. [Configuration Reference](#configuration-reference)
10. [Limitations and Future Work](#limitations-and-future-work)
11. [References](#references)
12. [License](#license)

---

## Overview

This system addresses the core perception requirements of an ADAS pipeline: understanding the ego vehicle's lane position, detecting and tracking surrounding traffic participants, estimating their kinematic state, and generating safety-critical collision warnings.

### Design Objectives

- **Real-time performance**: Sub-100 ms per-frame latency on consumer GPU hardware at 1280x720 resolution.
- **Modularity**: Each perception component (detection, tracking, estimation, warning) operates as an independent module with well-defined interfaces.
- **Dual-backend architecture**: A high-performance C++17 backend using OpenCV DNN for ONNX inference, and a Python backend using Ultralytics/PyTorch for rapid prototyping.
- **Production patterns**: Kalman-filtered tracking, temporal smoothing, hysteresis-based alert suppression, and configurable thresholds reflect patterns used in production ADAS stacks.

### Output Demo

https://github.com/Omjagdal/real-time-lane-vehicle-perception-system-for-adas-applications/blob/main/adas_54553913.mp4

---

## System Architecture

The system follows a pipelined architecture with clear separation between perception, estimation, and decision layers.

```
 ┌───────────────────────────────────────────────────────────────────┐
 │                        INPUT LAYER                                │
 │   Video Source (MP4/AVI/MOV) ──► Frame Acquisition (OpenCV)       │
 └──────────────────────────────┬────────────────────────────────────┘
                                │
 ┌──────────────────────────────▼────────────────────────────────────┐
 │                     PREPROCESSING LAYER                           │
 │   Resize (1280×720) ──► CLAHE ──► Gaussian Blur ──► ROI Masking   │
 │                    ──► Adaptive Canny Edge Detection               │
 └─────────┬────────────────────────────────────┬───────────────────┘
           │                                    │
 ┌─────────▼──────────────┐     ┌───────────────▼───────────────────┐
 │   LANE PERCEPTION       │     │   OBJECT PERCEPTION               │
 │                         │     │                                    │
 │   Hough Transform       │     │   YOLOv11n (ONNX / PyTorch)       │
 │   Slope Filtering       │     │   COCO Vehicle Class Filtering     │
 │   Outlier Rejection     │     │   NMS Post-Processing              │
 │   Temporal Smoothing    │     │   Confidence Thresholding          │
 │   Lane Departure Detect │     │                                    │
 └─────────┬───────────────┘     └───────────────┬───────────────────┘
           │                                     │
           │                     ┌───────────────▼───────────────────┐
           │                     │   TRACKING LAYER                   │
           │                     │                                    │
           │                     │   IoU Cost Matrix                  │
           │                     │   Hungarian Assignment             │
           │                     │   Kalman Filter Prediction         │
           │                     │   Track Lifecycle Management       │
           │                     └───────────────┬───────────────────┘
           │                                     │
           │                     ┌───────────────▼───────────────────┐
           │                     │   ESTIMATION LAYER                 │
           │                     │                                    │
           │                     │   Pinhole Camera Distance Model    │
           │                     │   Pixel-Displacement Speed Est.    │
           │                     │   EMA Temporal Smoothing            │
           │                     │   Perspective Correction            │
           │                     └───────────────┬───────────────────┘
           │                                     │
 ┌─────────▼─────────────────────────────────────▼───────────────────┐
 │                       DECISION LAYER                               │
 │                                                                    │
 │   Forward Collision Warning (FCW)                                  │
 │   Time-To-Collision (TTC) Computation                              │
 │   Three-Tier Alert Classification (SAFE / CAUTION / BRAKE)         │
 │   Hysteresis-Based Alert Suppression                               │
 └──────────────────────────────┬────────────────────────────────────┘
                                │
 ┌──────────────────────────────▼────────────────────────────────────┐
 │                       OUTPUT LAYER                                 │
 │   Annotated Frame Rendering ──► HUD Overlay ──► Minimap            │
 │   SSE Live Streaming ──► React Dashboard ──► MP4 Export            │
 └───────────────────────────────────────────────────────────────────┘
```

### Communication Architecture

```
 ┌──────────────┐     REST + SSE      ┌──────────────────────┐
 │  React 18    │ ◄──────────────────► │  FastAPI / C++ HTTP  │
 │  Vite 7      │   POST /api/upload   │  Server (Port 8000)  │
 │  Port 5173   │   GET  /api/process  │                      │
 │              │   GET  /api/frame    │  Pipeline Engine     │
 │  Recharts    │   GET  /api/download │  (OpenCV + YOLO)     │
 └──────────────┘                      └──────────────────────┘
```

---

## Perception Pipeline

### Frame Processing Sequence

Each video frame passes through the following stages in order:

| Stage | Module | Input | Output | Latency (GPU) |
|-------|--------|-------|--------|----------------|
| 1 | Preprocessing | Raw BGR frame | Resized frame + edge map | < 2 ms |
| 2 | Lane Detection | Canny edge image | Left/right lane polynomials | < 3 ms |
| 3 | Vehicle Detection | BGR frame (1280x720) | Bounding boxes + class labels | ~ 5 ms |
| 4 | Multi-Object Tracking | Detection list | Tracked objects with persistent IDs | < 1 ms |
| 5 | Distance Estimation | Tracked bounding boxes | Per-vehicle distance (metres) | < 0.5 ms |
| 6 | Speed Estimation | Distance time series | Per-vehicle speed (km/h) | < 0.5 ms |
| 7 | FCW Evaluation | Distances + speeds | Per-vehicle TTC + alert level | < 0.1 ms |
| 8 | Visualization | All perception outputs | Annotated frame with HUD | < 3 ms |

---

## Module Specifications

### 1. Preprocessing

Converts raw camera input into formats suitable for downstream perception modules.

| Operation | Method | Parameters |
|-----------|--------|------------|
| Resize | Bilinear interpolation | 1280 x 720 px |
| Contrast Enhancement | CLAHE (Contrast Limited Adaptive Histogram Equalization) | clip = 2.0, tile = 8x8 |
| Noise Reduction | Gaussian blur | kernel = 5x5 |
| Edge Detection | Adaptive Canny (Otsu-based thresholding) | auto low/high |
| ROI Masking | Trapezoidal region of interest | Bottom 42% of frame |

### 2. Lane Detection

Classical computer vision pipeline for detecting and stabilizing lane markings.

**Algorithm**:
1. Apply Probabilistic Hough Transform on the edge-detected ROI image.
2. Classify line segments as left or right by slope polarity.
3. Reject outliers beyond 1.5 sigma from the mean slope.
4. Compute length-weighted average of slope and intercept per side.
5. Apply temporal smoothing using a sliding window (N=10 frames, EMA alpha=0.3).
6. Extrapolate lane lines from the ROI boundary to the bottom of the frame.

**Lane Departure Detection**: Monitors the horizontal offset between the frame centre and the midpoint of the detected left and right lanes. Triggers `LEFT_DEPARTURE` or `RIGHT_DEPARTURE` when the offset exceeds a threshold.

| Parameter | Value |
|-----------|-------|
| Hough rho | 1 px |
| Hough threshold | 30 votes |
| Min line length | 40 px |
| Max line gap | 100 px |
| Slope range | [0.4, 10.0] |
| Smoothing window | 10 frames |

### 3. Vehicle Detection (YOLOv11n)

Real-time object detection using the YOLO v11 Nano architecture.

**Model**: YOLOv11n (Ultralytics) — 2.6M parameters, 5.8 MB (FP32).

**Inference Backends**:
- **C++ Backend**: OpenCV DNN module with ONNX model loading.
- **Python Backend**: Ultralytics API with PyTorch runtime.

**Post-processing**:
1. Filter detections to vehicle-relevant COCO classes: car (2), motorcycle (3), bus (5), truck (7).
2. Apply confidence threshold (default: 0.40).
3. Apply Non-Maximum Suppression with IoU threshold 0.45.
4. Compute bounding box centroid and area for downstream modules.

### 4. Multi-Object Tracking

IoU-based multi-object tracker with Kalman filter motion prediction and Hungarian algorithm assignment.

**Track Lifecycle**:

```
Detection ──► Tentative Track (hits < min_hits)
                    │
                    ▼ (matched for min_hits consecutive frames)
              Confirmed Track ──► Active Tracking
                    │
                    ▼ (unmatched for max_age frames)
              Track Deletion
```

| Parameter | Value | Description |
|-----------|-------|-------------|
| IoU threshold | 0.25 | Minimum overlap for assignment |
| Max age | 8 frames | Frames before track deletion |
| Min hits | 2 frames | Consecutive matches to confirm |
| BBox smoothing | EMA alpha = 0.5 | Reduces bounding box jitter |
| Kalman filter | 7-state linear model | Position + velocity + aspect ratio |

**Assignment**: The cost matrix `C[i][j] = 1.0 - IoU(track_i, detection_j)` is solved using the Hungarian algorithm (`scipy.linear_sum_assignment` in Python, custom implementation in C++).

### 5. Distance Estimation

Monocular distance estimation using the pinhole camera model with perspective correction.

**Formula**:

```
distance_raw = (real_width * focal_length) / pixel_width

correction = 1.0 - alpha * (cy - H/2) / (H/2)
distance = distance_raw * max(correction, 0.1)
distance = clamp(EMA_smooth(distance), 1.0, 200.0)
```

| Vehicle Class | Assumed Real Width |
|---------------|-------------------|
| Car | 1.8 m |
| Motorcycle | 0.8 m |
| Bus | 2.5 m |
| Truck | 2.4 m |

**Calibration assumptions**: Focal length = 850 px (for 1280x720), camera height = 1.3 m, pitch = 2 degrees.

### 6. Speed Estimation

Derives per-vehicle speed from frame-to-frame distance changes, smoothed with Exponential Moving Average.

**Method**:
1. Maintain a sliding window of distance measurements per track (N=10).
2. Compute speed from multi-frame displacement: `speed = delta_distance / delta_time`.
3. Reject implausible speed jumps (> 50 km/h between frames).
4. Apply EMA smoothing (alpha = 0.35).
5. Clamp output to [0, 250] km/h.

### 7. Forward Collision Warning (FCW)

Time-To-Collision (TTC) based warning system with three severity tiers and hysteresis-based alert suppression to prevent oscillation.

**TTC Computation**:

```
closing_speed = ego_speed - vehicle_speed    (m/s)
TTC = distance / closing_speed               (only if closing_speed > 0.5 m/s)
```

**Alert Classification** (aligned with ISO 15622 principles):

| Alert Level | TTC Condition | Distance Condition | Driver Action |
|-------------|---------------|--------------------|---------------|
| **BRAKE** | TTC < 1.5 s | OR distance < 10 m | Immediate braking required |
| **CAUTION** | TTC < 3.0 s | OR distance < 20 m | Prepare to decelerate |
| **SAFE** | TTC >= 3.0 s | AND distance >= 20 m | Maintain current speed |

**Hysteresis**: An alert level must persist for 3 consecutive frames before being promoted to prevent rapid flickering between states.

---

## Project Structure

```
.
├── cpp_backend/                          # High-Performance C++17 Backend
│   ├── CMakeLists.txt                    # CMake build configuration
│   ├── include/
│   │   ├── config.hpp                    # Pipeline parameters and constants
│   │   ├── preprocessing.hpp             # Frame preprocessing interface
│   │   ├── lane_detection.hpp            # Lane detection with temporal smoothing
│   │   ├── vehicle_detection.hpp         # YOLOv11n ONNX inference via OpenCV DNN
│   │   ├── tracker.hpp                   # IoU + Kalman multi-object tracker
│   │   ├── estimators.hpp                # Distance, speed, and FCW estimation
│   │   ├── visualization.hpp             # HUD, annotation, and minimap rendering
│   │   ├── httplib.h                     # cpp-httplib (embedded HTTP server)
│   │   └── nlohmann/json.hpp             # nlohmann/json (JSON serialization)
│   ├── src/
│   │   ├── main.cpp                      # HTTP server + pipeline orchestration
│   │   ├── preprocessing.cpp             # Resize, CLAHE, Canny, ROI
│   │   ├── lane_detection.cpp            # Hough Transform lane pipeline
│   │   ├── vehicle_detection.cpp         # ONNX model loading and inference
│   │   ├── tracker.cpp                   # Hungarian assignment + Kalman filter
│   │   ├── estimators.cpp                # Pinhole distance + TTC computation
│   │   └── visualization.cpp             # OpenCV drawing and HUD overlay
│   └── models/
│       └── yolo11n.onnx                  # YOLOv11n ONNX weights (10.7 MB)
│
├── src/                                  # Python Perception Modules
│   ├── __init__.py
│   ├── preprocessing.py                  # Frame preprocessing pipeline
│   ├── lane_detection.py                 # Hough Transform lane detection
│   ├── vehicle_detection.py              # Ultralytics YOLOv11n wrapper
│   ├── tracker.py                        # IoU multi-object tracker
│   ├── distance.py                       # Monocular distance estimation
│   ├── speed.py                          # Pixel-displacement speed estimation
│   ├── fcw.py                            # Forward Collision Warning engine
│   └── visualization.py                  # Annotated frame rendering
│
├── frontend/                             # React 18 + Vite Web Dashboard
│   ├── src/
│   │   ├── components/
│   │   │   ├── Header.jsx                # Navigation and pipeline step indicator
│   │   │   ├── IntroPage.jsx             # System overview and feature showcase
│   │   │   ├── UploadPage.jsx            # Video upload with pipeline configuration
│   │   │   ├── ProcessingPage.jsx        # Real-time SSE progress and live preview
│   │   │   ├── ResultsPage.jsx           # Summary analytics and video download
│   │   │   └── MetricCard.jsx            # Reusable metric display component
│   │   ├── App.jsx                       # State-machine page router
│   │   └── index.css                     # Design system (glassmorphism theme)
│   └── package.json
│
├── server.py                             # FastAPI REST + SSE backend
├── main.py                               # CLI pipeline runner
├── config.py                             # Python pipeline configuration
├── app.py                                # Streamlit alternative UI
├── Dockerfile                            # Container build specification
├── docker-compose.yml                    # Multi-service orchestration
├── requirements.txt                      # Python dependencies
└── README.md
```

---

## Build and Installation

### Prerequisites

| Dependency | Version | Purpose |
|------------|---------|---------|
| Python | 3.11+ | Python backend and CLI |
| Node.js | 18+ | React frontend build |
| CMake | 3.14+ | C++ backend build |
| OpenCV | 4.8+ | Computer vision and DNN inference |
| Git | 2.x | Version control |

### 1. Clone the Repository

```bash
git clone https://github.com/OmJagdale/Real-time-lane-Vehicle-Perception-system-for-ADAS-Applications.git
cd Real-time-lane-Vehicle-Perception-system-for-ADAS-Applications
```

### 2. C++ Backend (Recommended for Performance)

```bash
cd cpp_backend
mkdir -p build && cd build
cmake ..
make -j$(nproc)
cd ../..
```

**Dependencies**: OpenCV 4.8+ with DNN module (`brew install opencv` on macOS, `sudo apt install libopencv-dev` on Ubuntu).

### 3. Python Backend (Alternative)

```bash
python3.11 -m venv myenv
source myenv/bin/activate
pip install -r requirements.txt
pip install fastapi uvicorn python-multipart
```

### 4. Frontend

```bash
cd frontend
npm install
cd ..
```

---

## Usage

### Web Application (Recommended)

Start the backend (choose one):

```bash
# Option A: C++ Backend (high performance)
./cpp_backend/build/adas_server

# Option B: Python Backend
source myenv/bin/activate
uvicorn server:app --host 0.0.0.0 --port 8000
```

Start the frontend:

```bash
cd frontend
npm run dev
```

Open `http://localhost:5173` in your browser. Upload a dashcam video, configure pipeline parameters, and view real-time annotated results.

### Command-Line Interface

```bash
source myenv/bin/activate
python main.py --input data/dashcam.mp4 --output outputs/annotated.mp4 --device cuda --conf 0.4
```

| Argument | Default | Description |
|----------|---------|-------------|
| `--input` | required | Path to input video file |
| `--output` | required | Path to save annotated output |
| `--fps` | 30 | Target processing FPS |
| `--conf` | 0.4 | YOLO confidence threshold |
| `--device` | cpu | Inference device: `cpu`, `cuda`, `mps` |

### Docker

```bash
docker-compose up --build
```

---

## Performance Benchmarks

### Vehicle Detection (YOLOv11n)

| Metric | Value | Notes |
|--------|-------|-------|
| mAP@50 (COCO val) | 39.5% | All 80 COCO classes |
| mAP@50:95 (COCO val) | 27.3% | Standard COCO metric |
| Vehicle-filtered mAP@50 | 55 - 65% | Car, motorcycle, bus, truck only |
| Model size | 5.8 MB | FP32, nano variant |
| Parameters | 2.6 M | Smallest YOLO variant |
| Inference (GPU) | ~1.5 ms / frame | NVIDIA GTX 1060+, 640px input |
| Inference (CPU) | ~25 - 40 ms / frame | Intel i7 / Apple M1 |

### Multi-Object Tracking

| Metric | Expected Range | Notes |
|--------|----------------|-------|
| MOTA | 50 - 65% | IoU + Kalman, no appearance model |
| IDF1 | 55 - 70% | Identity preservation score |
| ID switches | Moderate | No ReID features; pure motion model |

### Lane Detection

| Metric | Value | Notes |
|--------|-------|-------|
| Detection rate | 70 - 80% | Well-marked highway lanes |
| Temporal stability | High | 10-frame smoothing window |
| Known failure modes | Sharp curves, faded markings, heavy shadows | Classical CV limitation |

### End-to-End Pipeline Throughput

| Configuration | FPS | Per-Frame Latency |
|---------------|-----|-------------------|
| C++ + GPU (1280x720) | 20 - 30 | < 50 ms |
| C++ + CPU (1280x720) | 8 - 15 | 70 - 130 ms |
| Python + GPU (1280x720) | 15 - 25 | < 80 ms |
| Python + CPU (1280x720) | 5 - 10 | 100 - 200 ms |
| Python + MPS (1280x720) | 10 - 18 | 60 - 100 ms |

---

## Configuration Reference

### Runtime Parameters (Web UI)

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| YOLO Confidence | 0.40 | 0.10 - 0.95 | Detection sensitivity threshold |
| Ego Speed | 60 km/h | 0 - 200 | Host vehicle speed for TTC computation |
| Max Frames | All | 0 - 5000 | Frame processing limit (0 = entire video) |
| Device | CPU | cpu / cuda / mps | Inference hardware backend |

### Pipeline Constants

| Parameter | File | Value | Unit |
|-----------|------|-------|------|
| `TARGET_WIDTH` | config.hpp | 1280 | px |
| `TARGET_HEIGHT` | config.hpp | 720 | px |
| `FOCAL_LENGTH` | config.hpp | 850.0 | px |
| `CAMERA_HEIGHT_M` | config.hpp | 1.3 | m |
| `LANE_SMOOTH_WINDOW` | config.hpp | 10 | frames |
| `IOU_THRESHOLD` | config.hpp | 0.25 | ratio |
| `MAX_AGE` | config.hpp | 8 | frames |
| `SPEED_EMA_ALPHA` | config.hpp | 0.35 | - |
| `TTC_BRAKE` | config.hpp | 1.5 | s |
| `TTC_CAUTION` | config.hpp | 3.0 | s |
| `FCW_HYSTERESIS_FRAMES` | config.hpp | 3 | frames |

---

## Limitations and Future Work

### Current Limitations

| Area | Limitation | Impact |
|------|-----------|--------|
| Distance estimation | Monocular, uncalibrated | +/- 20-30% error at range |
| Speed estimation | Derived from distance; errors compound | +/- 30-50% accuracy |
| Lane detection | Classical CV; no learned features | Fails on sharp curves, poor markings |
| Tracking | No appearance features (ReID) | ID switches during occlusion |
| Calibration | Assumed intrinsic parameters | System-specific calibration needed |

### Potential Extensions

- **Stereo or LiDAR fusion** for metric-accurate depth estimation.
- **Deep lane detection** (e.g., LaneATT, CLRNet) replacing Hough Transform for curved and multi-lane roads.
- **Appearance-based ReID** (e.g., DeepSORT, BoT-SORT) for robust tracking through occlusion.
- **Sensor fusion** with radar for velocity-independent distance measurement.
- **Camera calibration pipeline** for per-vehicle intrinsic/extrinsic parameter estimation.
- **Model optimization** via TensorRT (FP16/INT8 quantization) for embedded deployment.
- **Multi-camera surround view** extending perception to 360-degree coverage.
- **Functional safety** (ISO 26262) analysis for ASIL classification of FCW outputs.

---

## References

1. Redmon, J. et al. "You Only Look Once: Unified, Real-Time Object Detection." CVPR 2016.
2. Jocher, G. et al. "Ultralytics YOLO." https://github.com/ultralytics/ultralytics
3. Kuhn, H.W. "The Hungarian Method for the Assignment Problem." Naval Research Logistics, 1955.
4. Kalman, R.E. "A New Approach to Linear Filtering and Prediction Problems." ASME Journal of Basic Engineering, 1960.
5. ISO 15622:2018. "Intelligent transport systems - Adaptive cruise control systems - Performance requirements and test procedures."
6. Bradski, G. "The OpenCV Library." Dr. Dobb's Journal of Software Tools, 2000.
7. Canny, J. "A Computational Approach to Edge Detection." IEEE TPAMI, 1986.

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
