# ─────────────────────────────────────────────────────────────────
# ADAS Perception System — Multi-stage Docker Build
# ─────────────────────────────────────────────────────────────────

# Stage 1: Build React frontend
FROM node:20-slim AS frontend-build
WORKDIR /app/frontend
COPY frontend/package.json frontend/package-lock.json ./
RUN npm ci --production=false
COPY frontend/ ./
RUN npm run build

# Stage 2: Python backend
FROM python:3.11-slim AS runtime
WORKDIR /app

# System dependencies for OpenCV
RUN apt-get update && apt-get install -y --no-install-recommends \
    libgl1-mesa-glx libglib2.0-0 libsm6 libxrender1 libxext6 \
    && rm -rf /var/lib/apt/lists/*

# Python dependencies
COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

# Copy application code
COPY config.py server.py main.py app.py ./
COPY src/ ./src/
COPY models/ ./models/

# Copy built frontend
COPY --from=frontend-build /app/frontend/dist ./frontend/dist

# Create directories
RUN mkdir -p jobs logs outputs

# Environment
ENV ADAS_DEVICE=cpu
ENV ADAS_LOG_LEVEL=INFO

EXPOSE 8000

CMD ["uvicorn", "server:app", "--host", "0.0.0.0", "--port", "8000", "--workers", "1"]
