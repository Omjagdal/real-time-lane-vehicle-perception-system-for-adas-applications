import { useEffect, useState } from 'react'
import { BarChart, Bar, XAxis, YAxis, Tooltip, ResponsiveContainer, Cell } from 'recharts'
import { Download, RefreshCw, CheckCircle, Car, AlertTriangle, ShieldCheck } from 'lucide-react'
import axios from 'axios'
import MetricCard from './MetricCard'

const API = 'http://localhost:8000'

export default function ResultsPage({ jobId, onReset }) {
    const [job, setJob] = useState(null)

    useEffect(() => {
        axios.get(`${API}/api/jobs/${jobId}`).then(r => setJob(r.data))
    }, [jobId])

    if (!job) return (
        <div className="flex items-center justify-center min-h-[50vh]">
            <div
                className="w-10 h-10 rounded-full animate-spin"
                style={{
                    border: '2px solid rgba(6,182,212,0.2)',
                    borderTop: '2px solid #22d3ee',
                }}
            />
        </div>
    )

    const chartData = [
        { name: 'Frames', value: job.frame, fill: '#0891b2' },
        { name: 'Vehicles', value: job.num_vehicles, fill: '#6366f1' },
        { name: 'BRAKE', value: job.brake_events, fill: '#ef4444' },
        { name: 'CAUTION', value: job.caution_events, fill: '#f59e0b' },
    ]

    return (
        <div className="animate-fade-in space-y-6">

            {/* Success banner */}
            <div
                className="glass-elevated px-6 py-4 flex items-center gap-4"
                style={{
                    background: 'linear-gradient(135deg, rgba(16,185,129,0.06), rgba(16,185,129,0.02))',
                    border: '1px solid rgba(16,185,129,0.2)',
                }}
            >
                <div
                    className="w-10 h-10 rounded-xl flex items-center justify-center shrink-0"
                    style={{
                        background: 'linear-gradient(135deg, rgba(16,185,129,0.15), rgba(16,185,129,0.05))',
                        border: '1px solid rgba(16,185,129,0.25)',
                        boxShadow: '0 4px 12px rgba(16,185,129,0.1)',
                    }}
                >
                    <CheckCircle className="text-emerald-400" size={20} />
                </div>
                <div className="flex-1">
                    <p className="font-semibold text-white">Processing Complete</p>
                    <p className="text-sm text-gray-400 mt-0.5">
                        {job.frame} frames · {job.elapsed}s · {job.avg_fps} FPS average
                    </p>
                </div>
                <a
                    href={`${API}/api/download/${jobId}`}
                    download
                    className="btn-primary shrink-0"
                >
                    <Download size={16} /> Download Video
                </a>
            </div>

            {/* Metrics row */}
            <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
                <MetricCard icon={<Car size={18} />} label="Frames" value={job.frame} color="cyan" />
                <MetricCard icon={<Car size={18} />} label="Peak Vehicles" value={job.num_vehicles} color="indigo" />
                <MetricCard icon={<AlertTriangle size={18} />} label="BRAKE Events" value={job.brake_events} color="red" />
                <MetricCard icon={<ShieldCheck size={18} />} label="CAUTION Events" value={job.caution_events} color="amber" />
            </div>

            <div className="grid lg:grid-cols-5 gap-6">

                {/* Bar chart */}
                <div className="lg:col-span-3 glass-elevated p-6">
                    <p className="text-sm font-semibold text-gray-300 mb-5">Session Summary</p>
                    <ResponsiveContainer width="100%" height={200}>
                        <BarChart data={chartData} barCategoryGap="30%">
                            <XAxis dataKey="name" tick={{ fill: '#6b7280', fontSize: 12 }} axisLine={false} tickLine={false} />
                            <YAxis tick={{ fill: '#6b7280', fontSize: 11 }} axisLine={false} tickLine={false} />
                            <Tooltip
                                contentStyle={{
                                    background: 'rgba(15,23,42,0.95)',
                                    backdropFilter: 'blur(12px)',
                                    border: '1px solid rgba(255,255,255,0.1)',
                                    borderRadius: 12,
                                    color: '#fff',
                                    fontSize: 12,
                                    boxShadow: '0 8px 32px rgba(0,0,0,0.4)',
                                }}
                                cursor={{ fill: 'rgba(255,255,255,0.03)' }}
                            />
                            <Bar dataKey="value" radius={[8, 8, 0, 0]}>
                                {chartData.map((entry, i) => <Cell key={i} fill={entry.fill} fillOpacity={0.8} />)}
                            </Bar>
                        </BarChart>
                    </ResponsiveContainer>
                </div>

                {/* Job details */}
                <div className="lg:col-span-2 glass-elevated p-6 space-y-4">
                    <p className="text-sm font-semibold text-gray-300">Job Details</p>
                    <div className="space-y-3 text-sm">
                        {[
                            { label: 'Job ID', value: jobId.slice(0, 12) + '…' },
                            { label: 'File', value: job.filename || '—' },
                            { label: 'Source FPS', value: `${job.fps_source ?? '—'}` },
                            { label: 'Avg FPS', value: `${job.avg_fps} FPS` },
                            { label: 'Duration', value: `${job.elapsed}s` },
                            { label: 'Conf. Thresh', value: job.conf },
                            { label: 'Ego Speed', value: `${job.ego_speed} km/h` },
                            { label: 'Device', value: job.device },
                        ].map(r => (
                            <div key={r.label} className="flex justify-between items-center">
                                <span className="text-gray-500">{r.label}</span>
                                <span
                                    className="font-mono text-gray-200 text-xs px-2.5 py-1 rounded-lg"
                                    style={{
                                        background: 'rgba(255,255,255,0.04)',
                                        border: '1px solid rgba(255,255,255,0.06)',
                                    }}
                                >
                                    {r.value}
                                </span>
                            </div>
                        ))}
                    </div>

                    <div style={{ borderTop: '1px solid rgba(255,255,255,0.06)', paddingTop: '1rem' }}>
                        <a
                            href={`${API}/api/download/${jobId}`}
                            download
                            className="btn-primary w-full justify-center mb-3"
                        >
                            <Download size={15} /> Download Annotated MP4
                        </a>
                        <button onClick={onReset} className="btn-outline w-full justify-center">
                            <RefreshCw size={15} /> Process Another Video
                        </button>
                    </div>
                </div>
            </div>

        </div>
    )
}
