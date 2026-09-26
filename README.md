#  Real-Time Lane & Vehicle Perception System for ADAS Applications

An end-to-end **Advanced Driver Assistance System (ADAS)** perception pipeline that performs lane detection, vehicle detection, multi-object tracking, distance/speed estimation, and forward collision warning (FCW) in real time using dashcam or traffic camera video streams.

![Python](https://img.shields.io/badge/Python-3.11+-blue.svg)
![React](https://img.shields.io/badge/React-18+-61DAFB.svg)
![FastAPI](https://img.shields.io/badge/FastAPI-0.135+-009688.svg)
![OpenCV](https://img.shields.io/badge/OpenCV-4.8+-5C3EE8.svg)
![PyTorch](https://img.shields.io/badge/PyTorch-2.1+-EE4C2C.svg)
![YOLOv11](https://img.shields.io/badge/YOLOv11n-Ultralytics-00FFFF.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

---
Output Video



[Click here to watch the output video](https://github.com/Omjagdal/real-time-lane-vehicle-perception-system-for-adas-applications/blob/main/adas_54553913.mp4)

---

##  Key Features

| Feature | Description |
|---------|-------------|
|  **Real-Time Video Processing** | Upload dashcam/traffic videos and process frame-by-frame with live preview |
|  **Lane Detection** | Canny edge detection + Hough Line Transform with temporal smoothing |
|  **Vehicle Detection** | YOLOv11n (nano) — filters cars, motorcycles, buses, trucks |
|  **Multi-Object Tracking** | IoU-based tracker with Hungarian algorithm assignment & ID persistence |
|  **Monocular Distance Estimation** | Pinhole camera model with perspective correction |
|  **Speed Estimation** | Frame-to-frame pixel displacement with EMA smoothing |
| **Forward Collision Warning** | Time-To-Collision (TTC) based 3-tier alert system |
|  **Rich Visualization** | Annotated overlays — lanes, bounding boxes, distance, speed & FCW banners |
|  **Modern Web UI** | React + Vite frontend with SSE streaming & real-time dashboard |
|  **Video Download** | Download fully annotated MP4 output after processing |

---

##  System Architecture

```
                        ┌─────────────────────────┐
                        │    Input Video Stream    │
                        └────────────┬────────────┘
                                     ↓
                        ┌─────────────────────────┐
                        │    Preprocessing         │
                        │  Resize (1280×720)       │
                        │  Normalize / Color Conv  │
                        └─────┬──────────┬────────┘
                              ↓          ↓
                   ┌──────────────┐ ┌──────────────────┐
                   │ Lane         │ │ Vehicle Detection │
                   │ Detection    │ │ (YOLOv11n)        │
                   │ (Canny +     │ │ conf: 0.4         │
                   │  Hough)      │ │ NMS IoU: 0.45     │
                   └──────┬───────┘ └────────┬─────────┘
                          │                  ↓
                          │         ┌──────────────────┐
                          │         │ IoU Tracker       │
                          │         │ Hungarian Assign  │
                          │         │ ID Persistence    │
                          │         └────────┬─────────┘
                          │                  ↓
                          │         ┌──────────────────┐
                          │         │ Distance & Speed  │
                          │         │ Estimation        │
                          │         │ (Pinhole + EMA)   │
                          │         └────────┬─────────┘
                          │                  ↓
                          │         ┌──────────────────┐
                          │         │ FCW Engine        │
                          │         │ TTC Calculation   │
                          │         │ 3-Tier Alerts     │
                          │         └────────┬─────────┘
                          ↓                  ↓
                        ┌─────────────────────────┐
                        │    Visualization &       │
                        │    Annotated Output      │
                        └─────────────────────────┘
```

---

##  Project Structure

```
Real-time-lane-Vehicle-Perception-system-for-ADAS-Applications/
│
├── frontend/                        # React + Vite Web UI
│   ├── src/
│   │   ├── components/
│   │   │   ├── Header.jsx           # Navigation header
│   │   │   ├── UploadPage.jsx       # Video upload + settings panel
│   │   │   ├── ProcessingPage.jsx   # Live preview + SSE progress
│   │   │   ├── ResultsPage.jsx      # Summary charts + download
│   │   │   └── MetricCard.jsx       # Reusable metric display card
│   │   ├── App.jsx                  # Main app (state machine router)
│   │   ├── App.css                  # Component styles
│   │   ├── index.css                # Global styles + design system
│   │   └── main.jsx                 # Entry point
│   ├── index.html
│   └── package.json
│
├── src/                             # Core Python ADAS Pipeline
│   ├── __init__.py
│   ├── preprocessing.py             # Resize, grayscale, Canny, ROI masking
│   ├── lane_detection.py            # Hough Transform lane detection
│   ├── vehicle_detection.py         # YOLOv11n vehicle detector wrapper
│   ├── tracker.py                   # IoU multi-object tracker (Hungarian)
│   ├── distance.py                  # Monocular distance estimation
│   ├── speed.py                     # Pixel-displacement speed estimation
│   ├── fcw.py                       # Forward Collision Warning engine
│   └── visualization.py            # Drawing overlays, HUD, banners
│
├── models/
│   └── yolo/
│       └── yolov11n.pt              # YOLOv11 Nano weights
│
├── server.py                        # FastAPI backend (REST + SSE)
├── main.py                          # CLI pipeline runner
├── app.py                           # Streamlit app (alternative UI)
├── requirements.txt                 # Python dependencies
└── README.md
```

---

##  Installation

### Prerequisites

- **Python 3.11+**
- **Node.js 18+** and npm
- **Git**

### 1️ Clone the Repository

```bash
git clone https://github.com/OmJagdale/Real-time-lane-Vehicle-Perception-system-for-ADAS-Applications.git
cd Real-time-lane-Vehicle-Perception-system-for-ADAS-Applications
```

### 2️ Backend Setup (Python)

```bash
# Create and activate virtual environment
python3.11 -m venv myenv
source myenv/bin/activate        # Linux / macOS
myenv\Scripts\activate           # Windows

# Install all dependencies
pip install -r requirements.txt
pip install fastapi uvicorn python-multipart
```

### 3️ Frontend Setup (Node.js)

```bash
cd frontend
npm install
cd ..
```

---

## ▶️ How to Run

### 🖥️ Option 1: Full Web App (React + FastAPI)

The web app provides video upload, real-time SSE progress streaming, live annotated frame preview, and downloadable results.

**Terminal 1 — Start the API Backend:**
```bash
source myenv/bin/activate
uvicorn server:app --host 0.0.0.0 --port 8000
```

**Terminal 2 — Start the React Frontend:**
```bash
cd frontend
npm run dev
```

Open your browser at **`http://localhost:5173`**

### 💻 Option 2: CLI Pipeline

Process videos directly from the command line:
```bash
source myenv/bin/activate
python main.py --input data/input_video.mp4 --output outputs/annotated_video.mp4
```

**CLI Arguments:**
| Argument | Default | Description |
|----------|---------|-------------|
| `--input` | — | Path to input video file |
| `--output` | — | Path to save annotated output |
| `--fps` | 30 | Target FPS for processing |
| `--conf` | 0.4 | YOLO confidence threshold |
| `--device` | cpu | Inference device: `cpu`, `cuda`, `mps` |

### 🌐 Option 3: Streamlit App

```bash
source myenv/bin/activate
streamlit run app.py
```

---

##  Technologies Used

| Component | Technology | Version |
|-----------|-----------|---------|
| **Backend API** | FastAPI + Uvicorn | 0.135+ |
| **Frontend UI** | React + Vite + Recharts | React 18+ |
| **Styling** | Tailwind CSS + Custom CSS | — |
| **Computer Vision** | OpenCV | 4.8+ |
| **Deep Learning** | PyTorch | 2.1+ |
| **Object Detection** | YOLOv11n (Ultralytics) | 8.4+ |
| **Tracking** | IoU + SciPy Hungarian (linear_sum_assignment) | — |
| **Streaming** | Server-Sent Events (SSE) | — |
| **HTTP Client** | Axios | — |

---

##  Algorithm Details

### 1. Lane Detection (Traditional CV)

| Step | Method | Parameters |
|------|--------|------------|
| Preprocessing | Resize → Grayscale → Gaussian Blur (5×5) | — |
| Edge Detection | Canny | threshold1=50, threshold2=150 |
| ROI Masking | Trapezoidal mask (lower 42% of frame) | — |
| Line Detection | Probabilistic Hough Transform | threshold=30, minLen=40, maxGap=100 |
| Filtering | Slope-based (left/right separation) | slope ∈ [0.4, 10.0] |
| Smoothing | Length-weighted average + 8-frame temporal rolling | — |

### 2. Vehicle Detection (YOLOv11n)

```
Input Frame (1280×720 BGR)
       ↓
  YOLOv11n Inference
       ↓
  Filter COCO Vehicle Classes
    ├── Class 2: Car
    ├── Class 3: Motorcycle
    ├── Class 5: Bus
    └── Class 7: Truck
       ↓
  Apply Confidence Threshold (0.4)
       ↓
  Apply NMS (IoU 0.45)
       ↓
  Output: List[Detection(bbox, class_id, label, confidence)]
```

### 3. Multi-Object Tracking (IoU Tracker)

- **Assignment**: Hungarian algorithm on IoU cost matrix
- **IoU Threshold**: 0.30 (minimum overlap for match)
- **Max Age**: 5 frames (track survives without match)
- **Min Hits**: 2 frames (track confirmed after consecutive matches)
- **Output**: Persistent `track_id` across frames

### 4. Distance Estimation (Pinhole Camera Model)

```
Distance (m) = (Real_Width × Focal_Length) / Pixel_Width

Perspective Correction:
  correction = 1.0 - 0.5 × (cy - frame_height/2) / (frame_height/2)
  distance *= max(correction, 0.1)

Known Real Widths:
  Car: 1.8m | Motorcycle: 0.8m | Bus: 2.5m | Truck: 2.4m

Focal Length: 850 pixels (assumed for 1280×720)
Output Range: [1.0, 200.0] metres (clamped)
```

### 5. Speed Estimation (Pixel Displacement + EMA)

```
pixel_displacement = √((x₂-x₁)² + (y₂-y₁)²)
scale = 153 px/m × (10m / distance)
displacement_m = pixel_displacement / scale
speed_kmh = displacement_m × FPS × 3.6

Smoothing: EMA with α = 0.4
Output Range: [0.0, 250.0] km/h (clamped)
```

### 6. Forward Collision Warning (TTC)

```
TTC = Distance / Closing_Speed
Closing_Speed = Ego_Speed - Vehicle_Speed  (only if > 0.5 m/s)
```

| Alert Level | Trigger Condition | Action |
|-------------|-------------------|--------|
| 🛑 **BRAKE** | TTC < 1.5s **or** distance < 10m | Immediate braking required |
| ⚠️ **CAUTION** | TTC < 3.0s **or** distance < 20m | Prepare to decelerate |
| ✅ **SAFE** | TTC ≥ 3.0s **and** distance ≥ 20m | Maintain current speed |

---

##  Performance & Accuracy Metrics

### Vehicle Detection — YOLOv11n

| Metric | Value | Notes |
|--------|-------|-------|
| mAP@50 (COCO) | ~39.5% | Across all 80 COCO classes |
| mAP@50:95 (COCO) | ~27.3% | Standard COCO metric |
| Vehicle-specific mAP | ~55–65% | Only 4 filtered classes |
| Inference Speed (GPU) | ~1.5 ms/frame | At 640px input |
| Inference Speed (CPU) | ~25–40 ms/frame | At 640px input |
| Model Size | ~5.8 MB | Nano variant |

### Multi-Object Tracking

| Metric | Expected Range | Notes |
|--------|----------------|-------|
| MOTA | ~50–65% | IoU-only, no motion model |
| IDF1 | ~55–70% | Identity preservation score |
| ID Switches | Moderate | No ReID features |

### Lane Detection

| Metric | Value | Notes |
|--------|-------|-------|
| Detection Rate | ~70–80% | Well-marked highway lanes |
| Temporal Stability | High | 8-frame smoothing window |
| Failure Cases | Curves, faded markings, shadows | Classical CV limitation |

### Distance & Speed Estimation

| Module | Accuracy | Notes |
|--------|----------|-------|
| Distance | ±20–30% | Monocular, uncalibrated focal length |
| Speed | ±30–50% | Derived from distance; errors compound |

### End-to-End Pipeline

| Config | FPS | Notes |
|--------|-----|-------|
| CPU (1280×720) | 5–10 FPS | Apple M1/M2, Intel i7 |
| GPU (1280×720) | 15–25 FPS | NVIDIA GTX 1060+ |
| MPS (1280×720) | 10–18 FPS | Apple Silicon (M1/M2) |
| Latency/frame | < 100 ms | GPU; ~150–200 ms on CPU |

> **Note**: YOLOv11n is the smallest/fastest variant — optimized for speed over accuracy. Upgrade to `yolov11s` (small) or `yolov11m` (medium) for better detection precision at the cost of speed.

---

## Configuration

### Web UI Settings (Adjustable via slider)

| Setting | Default | Range | Description |
|---------|---------|-------|-------------|
| YOLO Confidence | 0.40 | 0.10 – 0.95 | Detection sensitivity |
| Ego Speed | 60 km/h | 0 – 200 | Assumed vehicle speed for TTC |
| Max Frames | All | 0 – 5000 | 0 = process entire video |
| Device | CPU | cpu / cuda / mps | Inference hardware |

### Pipeline Constants (in source code)

| Parameter | File | Value |
|-----------|------|-------|
| `TARGET_WIDTH` | preprocessing.py | 1280 px |
| `TARGET_HEIGHT` | preprocessing.py | 720 px |
| `FOCAL_LENGTH` | distance.py | 850 px |
| `SMOOTH_WINDOW` | lane_detection.py | 8 frames |
| `IOU_THRESHOLD` | tracker.py | 0.30 |
| `MAX_AGE` | tracker.py | 5 frames |
| `EMA_ALPHA` | speed.py | 0.4 |

---

## Troubleshooting

### YOLO model not loading
```bash
pip install ultralytics
# The model auto-downloads on first use if not found locally
```

### `ModuleNotFoundError: No module named 'scipy'`
```bash
pip install scipy
```

### `python-multipart` error
```bash
pip install python-multipart
```

### Low FPS
- Select **CUDA** or **MPS** device in the Web UI
- Reduce `TARGET_WIDTH`/`TARGET_HEIGHT` in `src/preprocessing.py`
- Increase YOLO confidence threshold to reduce detections
- Limit `max_frames` to process fewer frames

### Port already in use
```bash
# Kill existing process on port 8000
lsof -ti:8000 | xargs kill -9
```

---

##  Applications

-  Advanced Driver Assistance Systems (ADAS)
-  Autonomous driving perception research
-  Traffic monitoring & analytics
-  Smart transportation systems
-  Driver safety research & education
-  Computer vision portfolio projects


##  License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.

