import { useState, useCallback } from 'react'
import { Upload, Video, Settings2, ChevronRight, AlertCircle } from 'lucide-react'
import axios from 'axios'

const API = 'http://localhost:8000'

export default function UploadPage({ settings, setSettings, onJob }) {
    const [file, setFile] = useState(null)
    const [dragging, setDrag] = useState(false)
    const [loading, setLoading] = useState(false)
    const [error, setError] = useState(null)

    /* ---------- drag & drop ---------- */
    const onDrop = useCallback(e => {
        e.preventDefault(); setDrag(false)
        const f = e.dataTransfer.files[0]
        if (f && f.type.startsWith('video/')) setFile(f)
    }, [])
    const onDragOver = e => { e.preventDefault(); setDrag(true) }
    const onDragLeave = () => setDrag(false)
    const onFileInput = e => { const f = e.target.files[0]; if (f) setFile(f) }

    /* ---------- upload ---------- */
    async function handleStart() {
        if (!file) return
        setLoading(true); setError(null)
        try {
            const fd = new FormData()
            fd.append('file', file)
            fd.append('conf', settings.conf)
            fd.append('device', settings.device)
            fd.append('ego_speed', settings.ego_speed)
            fd.append('max_frames', settings.max_frames)
            const { data } = await axios.post(`${API}/api/upload`, fd)
            onJob(data.job_id)
        } catch (e) {
            setError(e.response?.data?.detail || 'Upload failed. Is the API server running?')
        } finally {
            setLoading(false)
        }
    }

    return (
        <div className="animate-fade-in space-y-8">

            {/* Hero text */}
            <div className="text-center space-y-3 pt-4">
                <h1 className="text-3xl md:text-4xl font-bold tracking-tight">
                    <span className="text-white">Real-Time </span>
                    <span
                        className="bg-clip-text text-transparent"
                        style={{ backgroundImage: 'linear-gradient(135deg, #22d3ee, #818cf8)' }}
                    >
                        ADAS Perception
                    </span>
                </h1>
                <p className="text-gray-400 text-sm max-w-xl mx-auto">
                    Lane detection · Vehicle tracking · Distance estimation · Forward Collision Warning
                </p>

                {/* Feature pills */}
                <div className="flex flex-wrap justify-center gap-2 pt-1">
                    {['YOLOv11n', 'IoU Tracking', 'TTC · FCW', 'Hough Lanes', 'Speed Estimation'].map(t => (
                        <span
                            key={t}
                            className="px-3 py-1 text-xs rounded-full text-gray-400"
                            style={{
                                background: 'rgba(255,255,255,0.04)',
                                border: '1px solid rgba(255,255,255,0.08)',
                            }}
                        >
                            {t}
                        </span>
                    ))}
                </div>
            </div>

            <div className="grid lg:grid-cols-5 gap-6 items-start">

                {/* Drop zone */}
                <div className="lg:col-span-3 space-y-5">
                    <div
                        onDrop={onDrop} onDragOver={onDragOver} onDragLeave={onDragLeave}
                        onClick={() => document.getElementById('file-input').click()}
                        className={`cursor-pointer p-10 flex flex-col items-center justify-center gap-5 min-h-[280px] transition-all duration-300 rounded-2xl ${dragging ? 'drop-active' : ''}`}
                        style={{
                            background: file
                                ? 'linear-gradient(135deg, rgba(16,185,129,0.06), rgba(16,185,129,0.02))'
                                : 'linear-gradient(135deg, rgba(255,255,255,0.04), rgba(255,255,255,0.01))',
                            border: file
                                ? '2px solid rgba(16,185,129,0.3)'
                                : dragging
                                    ? '2px solid rgba(6,182,212,0.5)'
                                    : '2px dashed rgba(255,255,255,0.1)',
                            backdropFilter: 'blur(20px)',
                            boxShadow: '0 4px 30px rgba(0,0,0,0.3), 0 1px 0 rgba(255,255,255,0.04) inset',
                        }}
                    >
                        <input id="file-input" type="file" accept="video/*"
                            className="hidden" onChange={onFileInput} />

                        {file ? (
                            <>
                                <div
                                    className="w-16 h-16 rounded-2xl flex items-center justify-center"
                                    style={{
                                        background: 'linear-gradient(135deg, rgba(16,185,129,0.15), rgba(16,185,129,0.05))',
                                        border: '1px solid rgba(16,185,129,0.25)',
                                        boxShadow: '0 4px 16px rgba(16,185,129,0.1)',
                                    }}
                                >
                                    <Video size={28} className="text-emerald-400" />
                                </div>
                                <div className="text-center">
                                    <p className="font-semibold text-white">{file.name}</p>
                                    <p className="text-sm text-gray-500 mt-1">
                                        {(file.size / 1e6).toFixed(1)} MB · Click to change
                                    </p>
                                </div>
                            </>
                        ) : (
                            <>
                                <div
                                    className={`w-16 h-16 rounded-2xl flex items-center justify-center transition-all duration-300 ${dragging ? 'scale-110' : ''}`}
                                    style={{
                                        background: dragging
                                            ? 'rgba(6,182,212,0.15)'
                                            : 'rgba(255,255,255,0.04)',
                                        border: '1px solid rgba(255,255,255,0.08)',
                                        boxShadow: '0 4px 16px rgba(0,0,0,0.2)',
                                    }}
                                >
                                    <Upload size={28} className={dragging ? 'text-cyan-400' : 'text-gray-500'} />
                                </div>
                                <div className="text-center">
                                    <p className="font-medium text-gray-300">Drop your dashcam video here</p>
                                    <p className="text-sm text-gray-600 mt-1">or click to browse · MP4, AVI, MOV, MKV</p>
                                </div>
                            </>
                        )}
                    </div>

                    {error && (
                        <div
                            className="flex items-center gap-2 px-4 py-3 rounded-xl text-sm"
                            style={{
                                background: 'rgba(239,68,68,0.08)',
                                border: '1px solid rgba(239,68,68,0.25)',
                                color: '#f87171',
                            }}
                        >
                            <AlertCircle size={15} />
                            {error}
                        </div>
                    )}

                    <button
                        onClick={handleStart}
                        disabled={!file || loading}
                        className="btn-primary w-full justify-center py-4 text-base disabled:opacity-40 disabled:cursor-not-allowed disabled:hover:translate-y-0"
                    >
                        {loading ? (
                            <><span className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin" /> Uploading…</>
                        ) : (
                            <><ChevronRight size={18} /> Start ADAS Pipeline</>
                        )}
                    </button>
                </div>

                {/* Settings panel */}
                <div className="lg:col-span-2 glass-elevated p-6 space-y-6">
                    <div className="flex items-center gap-2 text-sm font-semibold text-gray-300">
                        <Settings2 size={15} className="text-cyan-400" />
                        Pipeline Settings
                    </div>

                    {/* YOLO confidence */}
                    <div>
                        <div className="flex justify-between text-sm mb-3">
                            <span className="text-gray-400">YOLO Confidence</span>
                            <span className="font-mono text-cyan-400 text-xs px-2 py-0.5 rounded-md"
                                style={{ background: 'rgba(6,182,212,0.1)', border: '1px solid rgba(6,182,212,0.2)' }}>
                                {settings.conf.toFixed(2)}
                            </span>
                        </div>
                        <input type="range" min="0.1" max="0.95" step="0.05"
                            value={settings.conf}
                            onChange={e => setSettings(s => ({ ...s, conf: parseFloat(e.target.value) }))}
                        />
                        <div className="flex justify-between text-[10px] text-gray-600 mt-2">
                            <span>0.10 (sensitive)</span><span>0.95 (strict)</span>
                        </div>
                    </div>

                    {/* Ego speed */}
                    <div>
                        <div className="flex justify-between text-sm mb-3">
                            <span className="text-gray-400">Ego Speed</span>
                            <span className="font-mono text-cyan-400 text-xs px-2 py-0.5 rounded-md"
                                style={{ background: 'rgba(6,182,212,0.1)', border: '1px solid rgba(6,182,212,0.2)' }}>
                                {settings.ego_speed} km/h
                            </span>
                        </div>
                        <input type="range" min="0" max="200" step="5"
                            value={settings.ego_speed}
                            onChange={e => setSettings(s => ({ ...s, ego_speed: parseInt(e.target.value) }))}
                        />
                    </div>

                    {/* Max frames */}
                    <div>
                        <div className="flex justify-between text-sm mb-3">
                            <span className="text-gray-400">Max Frames</span>
                            <span className="font-mono text-cyan-400 text-xs px-2 py-0.5 rounded-md"
                                style={{ background: 'rgba(6,182,212,0.1)', border: '1px solid rgba(6,182,212,0.2)' }}>
                                {settings.max_frames === 0 ? 'All' : settings.max_frames}
                            </span>
                        </div>
                        <input type="range" min="0" max="5000" step="50"
                            value={settings.max_frames}
                            onChange={e => setSettings(s => ({ ...s, max_frames: parseInt(e.target.value) }))}
                        />
                        <div className="flex justify-between text-[10px] text-gray-600 mt-2">
                            <span>0 (all frames)</span><span>5000</span>
                        </div>
                    </div>

                    {/* Device */}
                    <div>
                        <p className="text-sm text-gray-400 mb-3">Inference Device</p>
                        <div className="grid grid-cols-3 gap-2">
                            {['cpu', 'cuda', 'mps'].map(d => (
                                <button key={d} onClick={() => setSettings(s => ({ ...s, device: d }))}
                                    className="py-2.5 rounded-xl text-xs font-semibold uppercase tracking-wider transition-all duration-300"
                                    style={{
                                        background: settings.device === d
                                            ? 'linear-gradient(135deg, rgba(6,182,212,0.15), rgba(99,102,241,0.1))'
                                            : 'rgba(255,255,255,0.03)',
                                        border: settings.device === d
                                            ? '1px solid rgba(6,182,212,0.4)'
                                            : '1px solid rgba(255,255,255,0.08)',
                                        color: settings.device === d ? '#22d3ee' : '#6b7280',
                                        boxShadow: settings.device === d
                                            ? '0 2px 12px rgba(6,182,212,0.15)'
                                            : 'none',
                                    }}>
                                    {d}
                                </button>
                            ))}
                        </div>
                    </div>

                    {/* Info */}
                    <div
                        className="pt-4 text-[11px] text-gray-600 space-y-1.5"
                        style={{ borderTop: '1px solid rgba(255,255,255,0.06)' }}
                    >
                        <p>BRAKE — TTC &lt; 1.5 s &nbsp;|&nbsp; distance &lt; 10 m</p>
                        <p>CAUTION — TTC &lt; 3.0 s &nbsp;|&nbsp; distance &lt; 20 m</p>
                        <p>SAFE — TTC ≥ 3.0 s &nbsp;|&nbsp; distance ≥ 20 m</p>
                    </div>
                </div>
            </div>
        </div>
    )
}
