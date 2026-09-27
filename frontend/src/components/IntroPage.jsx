import {
  ChevronRight, Car, Eye, Gauge, Shield, Radar, Zap,
  Cpu, GitBranch, Timer, Ruler, Activity, AlertTriangle,
  Github, Linkedin, Mail, ArrowRight
} from 'lucide-react'

const features = [
  {
    icon: <Eye size={20} />,
    title: 'Lane Detection',
    desc: 'Canny edge detection + Hough Transform with 8-frame temporal smoothing for stable lane overlay.',
    gradient: 'from-cyan-500 to-blue-600',
  },
  {
    icon: <Car size={20} />,
    title: 'Vehicle Detection',
    desc: 'YOLOv11n real-time inference filtering cars, trucks, buses, and motorcycles at 0.4 confidence.',
    gradient: 'from-indigo-500 to-violet-600',
  },
  {
    icon: <GitBranch size={20} />,
    title: 'Multi-Object Tracking',
    desc: 'IoU-based Hungarian assignment providing persistent identity across video frames.',
    gradient: 'from-violet-500 to-purple-600',
  },
  {
    icon: <Ruler size={20} />,
    title: 'Distance Estimation',
    desc: 'Pinhole camera model with perspective correction. Calibrated range: 1 to 200 metres.',
    gradient: 'from-emerald-500 to-teal-600',
  },
  {
    icon: <Gauge size={20} />,
    title: 'Speed Estimation',
    desc: 'Frame-to-frame pixel displacement with exponential moving average smoothing (alpha = 0.4).',
    gradient: 'from-amber-500 to-orange-600',
  },
  {
    icon: <Shield size={20} />,
    title: 'Collision Warning',
    desc: 'TTC-based three-tier FCW engine: SAFE, CAUTION, and BRAKE with real-time visual alerts.',
    gradient: 'from-red-500 to-rose-600',
  },
]

const pipelineSteps = [
  { label: 'Input', sub: 'Video Stream', icon: <Car size={16} /> },
  { label: 'Preprocess', sub: '1280×720', icon: <Cpu size={16} /> },
  { label: 'Detect', sub: 'YOLOv11n + Hough', icon: <Eye size={16} /> },
  { label: 'Track', sub: 'IoU + Hungarian', icon: <GitBranch size={16} /> },
  { label: 'Estimate', sub: 'Distance + Speed', icon: <Ruler size={16} /> },
  { label: 'FCW', sub: 'TTC Alerts', icon: <Shield size={16} /> },
  { label: 'Output', sub: 'Annotated Video', icon: <Activity size={16} /> },
]

const techStack = [
  { name: 'Python', version: '3.11+' },
  { name: 'React', version: '18+' },
  { name: 'FastAPI', version: '0.135+' },
  { name: 'PyTorch', version: '2.1+' },
  { name: 'OpenCV', version: '4.8+' },
  { name: 'YOLOv11n', version: 'Ultralytics' },
  { name: 'C++17', version: 'ONNX Backend' },
  { name: 'Vite', version: 'SSE Stream' },
]

const metrics = [
  { label: 'Vehicle mAP', value: '55–65%', sub: 'YOLOv11n filtered classes' },
  { label: 'Tracking MOTA', value: '50–65%', sub: 'IoU-based tracker' },
  { label: 'Lane Detection', value: '70–80%', sub: 'Highway accuracy' },
  { label: 'GPU FPS', value: '15–25', sub: '1280×720 resolution' },
]

export default function IntroPage({ onStart }) {
  return (
    <div className="animate-fade-in space-y-20 pb-16">

      {/* ── Hero Section ─── */}
      <section className="relative text-center pt-10 md:pt-20">
        {/* Background orbs */}
        <div className="absolute inset-0 overflow-hidden pointer-events-none">
          <div
            className="absolute top-[-10%] left-1/2 -translate-x-1/2 w-[700px] h-[700px] rounded-full animate-float"
            style={{ background: 'radial-gradient(circle, rgba(6,182,212,0.08) 0%, transparent 70%)' }}
          />
          <div
            className="absolute top-[20%] left-[20%] w-[400px] h-[400px] rounded-full animate-float"
            style={{ background: 'radial-gradient(circle, rgba(99,102,241,0.06) 0%, transparent 70%)', animationDelay: '2s' }}
          />
          <div
            className="absolute top-[10%] right-[10%] w-[300px] h-[300px] rounded-full animate-float"
            style={{ background: 'radial-gradient(circle, rgba(139,92,246,0.05) 0%, transparent 70%)', animationDelay: '4s' }}
          />
        </div>

        <div className="relative z-10 space-y-8">
          {/* Badge */}
          <div className="inline-flex items-center gap-2 px-4 py-1.5 rounded-full text-xs font-medium"
            style={{
              background: 'rgba(6,182,212,0.08)',
              border: '1px solid rgba(6,182,212,0.2)',
              color: '#22d3ee',
              boxShadow: '0 2px 12px rgba(6,182,212,0.1)',
            }}
          >
            <Radar size={13} className="animate-pulse" />
            Real-Time Perception Pipeline
          </div>

          {/* Title */}
          <h1 className="text-4xl sm:text-5xl md:text-6xl lg:text-7xl font-black tracking-tight leading-[1.08]">
            <span className="text-white">Advanced Driver</span>
            <br />
            <span
              className="bg-clip-text text-transparent"
              style={{ backgroundImage: 'linear-gradient(135deg, #22d3ee 0%, #818cf8 50%, #a78bfa 100%)' }}
            >
              Assistance System
            </span>
          </h1>

          {/* Subtitle */}
          <p className="text-gray-400 text-base md:text-lg max-w-2xl mx-auto leading-relaxed">
            End-to-end perception from raw dashcam footage to actionable safety alerts — 
            powered by deep learning and classical computer vision.
          </p>

          {/* CTA Buttons */}
          <div className="flex flex-col sm:flex-row gap-4 justify-center pt-2">
            <button onClick={onStart} className="btn-primary px-8 py-4 text-base">
              <Zap size={18} />
              Launch Pipeline
              <ArrowRight size={16} />
            </button>
            <a
              href="https://github.com/OmJagdale/Real-time-lane-Vehicle-Perception-system-for-ADAS-Applications"
              target="_blank"
              rel="noopener noreferrer"
              className="btn-outline px-7 py-3.5 text-base"
            >
              <Github size={16} />
              View Source
            </a>
          </div>

          {/* Quick stats */}
          <div className="flex flex-wrap justify-center gap-8 pt-4">
            {[
              { val: '6', label: 'CV Modules' },
              { val: 'YOLOv11n', label: 'Detection' },
              { val: 'Real-Time', label: 'Processing' },
              { val: '3-Tier', label: 'FCW Alerts' },
            ].map(s => (
              <div key={s.label} className="text-center">
                <p className="text-lg font-bold text-white font-mono">{s.val}</p>
                <p className="text-[11px] text-gray-500 uppercase tracking-widest mt-0.5">{s.label}</p>
              </div>
            ))}
          </div>
        </div>
      </section>

      {/* ── Pipeline Architecture ─── */}
      <section className="space-y-8">
        <div className="text-center space-y-2">
          <h2 className="text-2xl md:text-3xl font-bold text-white">Processing Pipeline</h2>
          <p className="text-gray-500 text-sm">End-to-end perception from raw video to annotated output</p>
        </div>

        <div className="glass-elevated p-6 md:p-8 overflow-x-auto">
          <div className="flex items-center justify-between min-w-[700px] gap-1">
            {pipelineSteps.map((step, i) => (
              <div key={step.label} className="flex items-center">
                <div className="flex flex-col items-center text-center group cursor-default">
                  <div
                    className="w-14 h-14 rounded-2xl flex items-center justify-center text-cyan-400 group-hover:text-white transition-all duration-300 group-hover:scale-110"
                    style={{
                      background: 'linear-gradient(135deg, rgba(255,255,255,0.06) 0%, rgba(255,255,255,0.02) 100%)',
                      border: '1px solid rgba(255,255,255,0.08)',
                      boxShadow: '0 4px 16px rgba(0,0,0,0.2), 0 1px 0 rgba(255,255,255,0.04) inset',
                    }}
                  >
                    {step.icon}
                  </div>
                  <p className="text-xs font-semibold text-gray-300 mt-2.5">{step.label}</p>
                  <p className="text-[10px] text-gray-600 mt-0.5">{step.sub}</p>
                </div>
                {i < pipelineSteps.length - 1 && (
                  <div className="flex items-center mx-2 mt-[-24px]">
                    <div
                      className="w-8 md:w-12 h-px"
                      style={{ background: 'linear-gradient(90deg, rgba(6,182,212,0.3), rgba(99,102,241,0.3))' }}
                    />
                    <ChevronRight size={10} className="text-cyan-500/40 -ml-1" />
                  </div>
                )}
              </div>
            ))}
          </div>
        </div>
      </section>

      {/* ── Feature Cards ─── */}
      <section className="space-y-8">
        <div className="text-center space-y-2">
          <h2 className="text-2xl md:text-3xl font-bold text-white">Core Modules</h2>
          <p className="text-gray-500 text-sm">Six tightly integrated perception components</p>
        </div>

        <div className="grid sm:grid-cols-2 lg:grid-cols-3 gap-5">
          {features.map((f, i) => (
            <div
              key={f.title}
              className="glass-elevated p-6 space-y-4 cursor-default group"
              style={{ animationDelay: `${i * 100}ms` }}
            >
              <div
                className={`w-10 h-10 rounded-xl bg-gradient-to-br ${f.gradient} flex items-center justify-center text-white group-hover:scale-110 transition-all duration-300`}
                style={{
                  boxShadow: '0 4px 16px rgba(0,0,0,0.3), 0 1px 0 rgba(255,255,255,0.2) inset',
                }}
              >
                {f.icon}
              </div>
              <h3 className="font-semibold text-white">{f.title}</h3>
              <p className="text-sm text-gray-400 leading-relaxed">{f.desc}</p>
            </div>
          ))}
        </div>
      </section>

      {/* ── FCW Demo ─── */}
      <section className="space-y-8">
        <div className="text-center space-y-2">
          <h2 className="text-2xl md:text-3xl font-bold text-white">Forward Collision Warning</h2>
          <p className="text-gray-500 text-sm">Three-tier Time-To-Collision based alert system</p>
        </div>

        <div className="grid md:grid-cols-3 gap-5">
          {/* SAFE */}
          <div className="glass-elevated p-6 text-center space-y-4">
            <div
              className="w-14 h-14 rounded-2xl flex items-center justify-center mx-auto"
              style={{
                background: 'linear-gradient(135deg, rgba(16,185,129,0.15), rgba(16,185,129,0.05))',
                border: '1px solid rgba(16,185,129,0.25)',
                boxShadow: '0 4px 20px rgba(16,185,129,0.1), 0 1px 0 rgba(255,255,255,0.05) inset',
              }}
            >
              <Shield size={22} className="text-emerald-400" />
            </div>
            <h3 className="text-emerald-400 font-bold text-lg">SAFE</h3>
            <div className="space-y-1">
              <p className="text-sm text-gray-400">TTC ≥ 3.0 seconds</p>
              <p className="text-sm text-gray-400">Distance ≥ 20 metres</p>
            </div>
            <p className="text-xs text-gray-600">Maintain current speed</p>
          </div>

          {/* CAUTION */}
          <div className="glass-elevated p-6 text-center space-y-4">
            <div
              className="w-14 h-14 rounded-2xl flex items-center justify-center mx-auto"
              style={{
                background: 'linear-gradient(135deg, rgba(245,158,11,0.15), rgba(245,158,11,0.05))',
                border: '1px solid rgba(245,158,11,0.25)',
                boxShadow: '0 4px 20px rgba(245,158,11,0.1), 0 1px 0 rgba(255,255,255,0.05) inset',
              }}
            >
              <AlertTriangle size={22} className="text-amber-400" />
            </div>
            <h3 className="text-amber-400 font-bold text-lg">CAUTION</h3>
            <div className="space-y-1">
              <p className="text-sm text-gray-400">TTC &lt; 3.0 seconds</p>
              <p className="text-sm text-gray-400">Distance &lt; 20 metres</p>
            </div>
            <p className="text-xs text-gray-600">Prepare to decelerate</p>
          </div>

          {/* BRAKE */}
          <div className="glass-elevated p-6 text-center space-y-4">
            <div
              className="w-14 h-14 rounded-2xl flex items-center justify-center mx-auto animate-pulse-slow"
              style={{
                background: 'linear-gradient(135deg, rgba(239,68,68,0.15), rgba(239,68,68,0.05))',
                border: '1px solid rgba(239,68,68,0.25)',
                boxShadow: '0 4px 20px rgba(239,68,68,0.15), 0 1px 0 rgba(255,255,255,0.05) inset',
              }}
            >
              <AlertTriangle size={22} className="text-red-400" />
            </div>
            <h3 className="text-red-400 font-bold text-lg">BRAKE</h3>
            <div className="space-y-1">
              <p className="text-sm text-gray-400">TTC &lt; 1.5 seconds</p>
              <p className="text-sm text-gray-400">Distance &lt; 10 metres</p>
            </div>
            <p className="text-xs text-gray-600">Immediate braking required</p>
          </div>
        </div>

        {/* TTC formula */}
        <div
          className="glass p-4 text-center"
          style={{ maxWidth: '480px', margin: '0 auto' }}
        >
          <p className="text-[11px] text-gray-500 mb-1.5 uppercase tracking-wider">TTC Formula</p>
          <p className="font-mono text-sm text-cyan-400">
            TTC = Distance / (Ego_Speed − Vehicle_Speed)
          </p>
        </div>
      </section>

      {/* ── Performance Metrics ─── */}
      <section className="space-y-8">
        <div className="text-center space-y-2">
          <h2 className="text-2xl md:text-3xl font-bold text-white">Performance Metrics</h2>
          <p className="text-gray-500 text-sm">Benchmarked on real-world dashcam footage</p>
        </div>

        <div className="grid grid-cols-2 md:grid-cols-4 gap-5">
          {metrics.map(m => (
            <div key={m.label} className="glass-elevated p-6 text-center space-y-3">
              <p
                className="text-2xl md:text-3xl font-bold font-mono bg-clip-text text-transparent"
                style={{ backgroundImage: 'linear-gradient(135deg, #22d3ee, #818cf8)' }}
              >
                {m.value}
              </p>
              <p className="text-sm font-semibold text-gray-300">{m.label}</p>
              <p className="text-[11px] text-gray-600">{m.sub}</p>
            </div>
          ))}
        </div>
      </section>

      {/* ── Tech Stack ─── */}
      <section className="space-y-8">
        <div className="text-center space-y-2">
          <h2 className="text-2xl md:text-3xl font-bold text-white">Technology Stack</h2>
          <p className="text-gray-500 text-sm">Built with industry-standard tools</p>
        </div>

        <div className="grid grid-cols-2 sm:grid-cols-4 gap-4">
          {techStack.map(t => (
            <div key={t.name} className="glass p-4 flex items-center gap-3 cursor-default group">
              <div
                className="w-9 h-9 rounded-lg flex items-center justify-center text-cyan-400 group-hover:text-white transition-colors shrink-0"
                style={{
                  background: 'linear-gradient(135deg, rgba(6,182,212,0.1), rgba(99,102,241,0.1))',
                  border: '1px solid rgba(255,255,255,0.08)',
                }}
              >
                <Cpu size={16} />
              </div>
              <div>
                <p className="text-sm font-semibold text-white">{t.name}</p>
                <p className="text-[10px] text-gray-500">{t.version}</p>
              </div>
            </div>
          ))}
        </div>
      </section>

      {/* ── Algorithm Details ─── */}
      <section className="space-y-8">
        <div className="text-center space-y-2">
          <h2 className="text-2xl md:text-3xl font-bold text-white">How It Works</h2>
          <p className="text-gray-500 text-sm">Key algorithms powering the pipeline</p>
        </div>

        <div className="grid md:grid-cols-2 gap-5">
          {[
            {
              icon: <Ruler size={16} className="text-emerald-400" />,
              title: 'Pinhole Camera Distance',
              lines: [
                'Distance = (Real_Width × Focal_Length) / Pixel_Width',
                '// Focal Length: 850px (1280×720)',
                '// Car: 1.8m | Bus: 2.5m | Truck: 2.4m',
              ],
            },
            {
              icon: <Gauge size={16} className="text-amber-400" />,
              title: 'EMA Speed Estimation',
              lines: [
                'pixel_disp = √((x₂-x₁)² + (y₂-y₁)²)',
                'speed_kmh = (pixel_disp / scale) × FPS × 3.6',
                '// EMA smoothing: α = 0.4',
              ],
            },
            {
              icon: <Eye size={16} className="text-indigo-400" />,
              title: 'YOLOv11n Detection',
              lines: [
                'COCO Classes: [2: car, 3: moto, 5: bus, 7: truck]',
                'Conf: 0.4 | NMS IoU: 0.45',
                '// ~5.8 MB nano model · GPU: 1.5ms/frame',
              ],
            },
            {
              icon: <Activity size={16} className="text-violet-400" />,
              title: 'Hungarian IoU Tracking',
              lines: [
                'cost[i][j] = 1.0 - IoU(track_i, det_j)',
                'assignment = linear_sum_assignment(cost)',
                '// IoU ≥ 0.30 | Max age: 5 | Min hits: 2',
              ],
            },
          ].map(algo => (
            <div key={algo.title} className="glass-elevated p-5 space-y-3">
              <div className="flex items-center gap-2">
                {algo.icon}
                <h3 className="text-sm font-semibold text-white">{algo.title}</h3>
              </div>
              <div
                className="rounded-xl p-4 font-mono text-xs space-y-1 overflow-x-auto"
                style={{
                  background: 'rgba(0,0,0,0.3)',
                  border: '1px solid rgba(255,255,255,0.05)',
                  boxShadow: '0 2px 8px rgba(0,0,0,0.3) inset',
                }}
              >
                {algo.lines.map((line, i) => (
                  <p key={i} className={line.startsWith('//') ? 'text-gray-600' : 'text-cyan-300'}>
                    {line}
                  </p>
                ))}
              </div>
            </div>
          ))}
        </div>
      </section>

      {/* ── CTA + Author ─── */}
      <section className="text-center space-y-10 pt-4">
        <div
          className="glass-elevated p-10 md:p-14 space-y-5"
          style={{
            background: 'linear-gradient(135deg, rgba(6,182,212,0.06) 0%, rgba(99,102,241,0.04) 100%)',
            border: '1px solid rgba(6,182,212,0.12)',
          }}
        >
          <h2 className="text-2xl md:text-3xl font-bold text-white">Ready to Analyze?</h2>
          <p className="text-gray-400 max-w-lg mx-auto text-sm leading-relaxed">
            Upload your dashcam footage and watch the ADAS pipeline detect lanes,
            track vehicles, estimate distances, and generate collision warnings in real time.
          </p>
          <button onClick={onStart} className="btn-primary px-10 py-4 text-base mt-2">
            <Zap size={18} />
            Start Processing
            <ArrowRight size={16} />
          </button>
        </div>

        {/* Author */}
        <div className="space-y-3">
          <p className="text-sm text-gray-500">Computer Vision · AI · ADAS · Deep Learning · Edge AI</p>
          <div className="flex justify-center gap-3 pt-1">
            {[
              { href: 'https://linkedin.com/in/omjagdale', icon: <Linkedin size={14} /> },
              { href: 'https://github.com/OmJagdale', icon: <Github size={14} /> },
              { href: 'mailto:omjagdale.ai@gmail.com', icon: <Mail size={14} /> },
            ].map(link => (
              <a
                key={link.href}
                href={link.href}
                target="_blank"
                rel="noopener noreferrer"
                className="w-9 h-9 rounded-xl flex items-center justify-center text-gray-500 hover:text-cyan-400 transition-all duration-300 hover:scale-110"
                style={{
                  background: 'linear-gradient(135deg, rgba(255,255,255,0.04), rgba(255,255,255,0.01))',
                  border: '1px solid rgba(255,255,255,0.08)',
                  boxShadow: '0 2px 8px rgba(0,0,0,0.2)',
                }}
              >
                {link.icon}
              </a>
            ))}
          </div>
        </div>
      </section>
    </div>
  )
}
