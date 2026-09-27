import { useEffect, useRef, useState } from 'react'
import { Cpu, Car, AlertTriangle, ShieldCheck, Gauge } from 'lucide-react'
import MetricCard from './MetricCard'

const API = 'http://localhost:8000'

export default function ProcessingPage({ jobId, onDone }) {
    const [stats, setStats] = useState({
        status: 'queued', frame: 0, total_frames: 1,
        fps_live: 0, num_vehicles: 0, brake_events: 0, caution_events: 0,
    })
    const [frameUrl, setFrameUrl] = useState(null)
    const esRef = useRef(null)

    /* SSE stream */
    useEffect(() => {
        const es = new EventSource(`${API}/api/process/${jobId}`)
        esRef.current = es

        es.onmessage = e => {
            const data = JSON.parse(e.data)
            setStats(data)

            // Only refresh preview once at least one frame is processed
            if (data.frame > 0) {
                setFrameUrl(`${API}/api/frame/${jobId}?t=${Date.now()}`)
            }

            if (data.status === 'done') {
                es.close()
                setTimeout(onDone, 800)
            }
            if (data.status === 'error') {
                es.close()
            }
        }
        es.onerror = () => es.close()

        return () => es.close()
    }, [jobId])

    const pct = stats.total_frames > 0
        ? Math.round((stats.frame / stats.total_frames) * 100) : 0

    const statusStyles = {
        queued: { color: '#9ca3af', bg: 'rgba(156,163,175,0.1)', border: 'rgba(156,163,175,0.2)' },
        processing: { color: '#22d3ee', bg: 'rgba(6,182,212,0.1)', border: 'rgba(6,182,212,0.2)' },
        done: { color: '#34d399', bg: 'rgba(16,185,129,0.1)', border: 'rgba(16,185,129,0.2)' },
        error: { color: '#f87171', bg: 'rgba(239,68,68,0.1)', border: 'rgba(239,68,68,0.2)' },
    }
    const statusStyle = statusStyles[stats.status] || statusStyles.queued

    return (
        <div className="animate-fade-in space-y-6">

            {/* Title row */}
            <div className="flex items-center justify-between">
                <div>
                    <h2 className="text-2xl font-bold text-white">Processing Pipeline</h2>
                    <p className="text-gray-500 text-sm mt-1 font-mono">Job {jobId.slice(0, 8)}</p>
                </div>
                <span
                    className="text-xs font-semibold capitalize flex items-center gap-2 px-3 py-1.5 rounded-xl"
                    style={{
                        color: statusStyle.color,
                        background: statusStyle.bg,
                        border: `1px solid ${statusStyle.border}`,
                    }}
                >
                    {stats.status === 'processing' && (
                        <span className="w-2 h-2 rounded-full animate-pulse" style={{ background: '#22d3ee' }} />
                    )}
                    {stats.status}
                </span>
            </div>

            {/* Progress */}
            <div className="glass-elevated p-5 space-y-3">
                <div className="flex justify-between text-sm">
                    <span className="text-gray-400">
                        Frame <span className="font-mono text-white">{stats.frame}</span>
                        {' / '}
                        <span className="font-mono text-white">{stats.total_frames}</span>
                    </span>
                    <span className="font-mono font-bold text-cyan-400">{pct}%</span>
                </div>
                <div className="progress-track">
                    <div className="progress-bar" style={{ width: `${pct}%` }} />
                </div>
                <div className="flex justify-between text-xs text-gray-600">
                    <span>{stats.fps_live} FPS</span>
                    <span>
                        {stats.total_frames > 0 && stats.fps_live > 0
                            ? `~${Math.round((stats.total_frames - stats.frame) / stats.fps_live)}s remaining`
                            : '—'}
                    </span>
                </div>
            </div>

            {/* Two-column layout */}
            <div className="grid lg:grid-cols-5 gap-5">

                {/* Live Preview */}
                <div className="lg:col-span-3 glass-elevated overflow-hidden">
                    <div
                        className="px-4 py-3 flex items-center justify-between"
                        style={{ borderBottom: '1px solid rgba(255,255,255,0.06)' }}
                    >
                        <span className="text-sm font-medium text-gray-300">Live Preview</span>
                        {stats.status === 'processing' && (
                            <span className="badge-safe">LIVE</span>
                        )}
                    </div>
                    <div className="aspect-video flex items-center justify-center" style={{ background: 'rgba(0,0,0,0.3)' }}>
                        {frameUrl ? (
                            <img src={frameUrl} alt="Preview" className="w-full h-full object-contain"
                                onError={e => { e.target.style.display = 'none' }} />
                        ) : (
                            <div className="flex flex-col items-center gap-3 text-gray-600">
                                <Cpu size={32} className="animate-pulse" />
                                <p className="text-sm">Waiting for first frame…</p>
                            </div>
                        )}
                    </div>
                </div>

                {/* Stats column */}
                <div className="lg:col-span-2 grid grid-cols-2 gap-4 content-start">
                    <MetricCard
                        icon={<Gauge size={18} />}
                        label="Live FPS"
                        value={stats.fps_live}
                        color="cyan"
                        unit="fps"
                    />
                    <MetricCard
                        icon={<Car size={18} />}
                        label="Vehicles"
                        value={stats.num_vehicles}
                        color="indigo"
                    />
                    <MetricCard
                        icon={<AlertTriangle size={18} />}
                        label="BRAKE Events"
                        value={stats.brake_events}
                        color="red"
                    />
                    <MetricCard
                        icon={<ShieldCheck size={18} />}
                        label="CAUTION Events"
                        value={stats.caution_events}
                        color="amber"
                    />

                    {/* Alert legend */}
                    <div className="col-span-2 glass p-4 space-y-2.5 text-xs">
                        <p className="font-semibold text-gray-400 mb-3">FCW Thresholds</p>
                        <div className="flex items-center gap-2">
                            <span className="badge-brake">BRAKE</span>
                            <span className="text-gray-500">TTC &lt; 1.5 s · dist &lt; 10 m</span>
                        </div>
                        <div className="flex items-center gap-2">
                            <span className="badge-caution">CAUTION</span>
                            <span className="text-gray-500">TTC &lt; 3.0 s · dist &lt; 20 m</span>
                        </div>
                        <div className="flex items-center gap-2">
                            <span className="badge-safe">SAFE</span>
                            <span className="text-gray-500">TTC ≥ 3.0 s · dist ≥ 20 m</span>
                        </div>
                    </div>
                </div>
            </div>

            {stats.status === 'error' && (
                <div
                    className="glass p-4 text-sm"
                    style={{
                        background: 'rgba(239,68,68,0.06)',
                        border: '1px solid rgba(239,68,68,0.2)',
                        color: '#f87171',
                    }}
                >
                    Processing failed. Check the server logs for details.
                </div>
            )}
        </div>
    )
}
