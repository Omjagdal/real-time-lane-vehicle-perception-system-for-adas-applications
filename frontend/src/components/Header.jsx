import { Car, Activity } from 'lucide-react'

const steps = [
    { id: 'upload', label: 'Upload' },
    { id: 'processing', label: 'Processing' },
    { id: 'results', label: 'Results' },
]

export default function Header({ page, onHome }) {
    const current = steps.findIndex(s => s.id === page)
    const isIntro = page === 'intro'

    return (
        <header
            className="sticky top-0 z-50"
            style={{
                background: 'linear-gradient(180deg, rgba(5,10,21,0.85) 0%, rgba(5,10,21,0.7) 100%)',
                backdropFilter: 'blur(24px) saturate(1.5)',
                WebkitBackdropFilter: 'blur(24px) saturate(1.5)',
                borderBottom: '1px solid rgba(255,255,255,0.06)',
                boxShadow: '0 4px 30px rgba(0,0,0,0.3)',
            }}
        >
            <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 h-16 flex items-center justify-between gap-6">

                {/* Logo */}
                <button
                    onClick={onHome}
                    className="flex items-center gap-3 shrink-0 group transition-all duration-300"
                >
                    <div
                        className="w-10 h-10 rounded-xl flex items-center justify-center group-hover:scale-105 transition-all duration-300"
                        style={{
                            background: 'linear-gradient(135deg, #0891b2 0%, #6366f1 100%)',
                            border: '1px solid rgba(255,255,255,0.2)',
                            boxShadow: '0 4px 16px rgba(6,182,212,0.3), 0 1px 0 rgba(255,255,255,0.15) inset',
                        }}
                    >
                        <Car size={18} className="text-white" />
                    </div>
                    <div className="flex items-center gap-2">
                        <span className="font-bold text-white tracking-tight text-lg">ADAS</span>
                        <span
                            className="text-[10px] font-semibold uppercase tracking-[0.15em] text-cyan-400 px-2 py-0.5 rounded-md"
                            style={{
                                background: 'rgba(6,182,212,0.1)',
                                border: '1px solid rgba(6,182,212,0.25)',
                            }}
                        >
                            Perception
                        </span>
                    </div>
                </button>

                {/* Step indicator */}
                {!isIntro && (
                    <nav className="hidden sm:flex items-center gap-1">
                        {steps.map((step, i) => {
                            const done = i < current
                            const active = i === current
                            return (
                                <div key={step.id} className="flex items-center">
                                    <div
                                        className="flex items-center gap-2 px-3 py-1.5 rounded-xl text-sm font-medium transition-all duration-300"
                                        style={{
                                            background: active ? 'rgba(6,182,212,0.1)' : 'transparent',
                                            border: active ? '1px solid rgba(6,182,212,0.2)' : '1px solid transparent',
                                            color: active ? '#22d3ee' : done ? '#9ca3af' : '#4b5563',
                                        }}
                                    >
                                        <span
                                            className="w-5 h-5 rounded-full flex items-center justify-center text-[10px] font-bold transition-all"
                                            style={{
                                                background: active
                                                    ? 'linear-gradient(135deg, #0891b2, #6366f1)'
                                                    : done
                                                        ? 'rgba(255,255,255,0.15)'
                                                        : 'rgba(255,255,255,0.05)',
                                                color: active || done ? '#fff' : '#4b5563',
                                                boxShadow: active ? '0 2px 8px rgba(6,182,212,0.3)' : 'none',
                                            }}
                                        >
                                            {done ? '✓' : i + 1}
                                        </span>
                                        {step.label}
                                    </div>
                                    {i < steps.length - 1 && (
                                        <div
                                            className="w-8 h-px mx-1"
                                            style={{
                                                background: done
                                                    ? 'linear-gradient(90deg, rgba(6,182,212,0.5), rgba(99,102,241,0.5))'
                                                    : 'rgba(255,255,255,0.06)',
                                            }}
                                        />
                                    )}
                                </div>
                            )
                        })}
                    </nav>
                )}

                {/* Right side */}
                <div className="flex items-center gap-4">
                    {isIntro && (
                        <a
                            href="https://github.com/OmJagdale/Real-time-lane-Vehicle-Perception-system-for-ADAS-Applications"
                            target="_blank"
                            rel="noopener noreferrer"
                            className="hidden sm:inline-flex items-center gap-1.5 text-xs text-gray-500 hover:text-cyan-400 transition-colors duration-300"
                        >
                            <svg className="w-3.5 h-3.5" fill="currentColor" viewBox="0 0 24 24"><path d="M12 0C5.37 0 0 5.37 0 12c0 5.31 3.435 9.795 8.205 11.385.6.105.825-.255.825-.57 0-.285-.015-1.23-.015-2.235-3.015.555-3.795-.735-4.035-1.41-.135-.345-.72-1.41-1.23-1.695-.42-.225-1.02-.78-.015-.795.945-.015 1.62.87 1.845 1.23 1.08 1.815 2.805 1.305 3.495.99.105-.78.42-1.305.765-1.605-2.67-.3-5.46-1.335-5.46-5.925 0-1.305.465-2.385 1.23-3.225-.12-.3-.54-1.53.12-3.18 0 0 1.005-.315 3.3 1.23.96-.27 1.98-.405 3-.405s2.04.135 3 .405c2.295-1.56 3.3-1.23 3.3-1.23.66 1.65.24 2.88.12 3.18.765.84 1.23 1.905 1.23 3.225 0 4.605-2.805 5.625-5.475 5.925.435.375.81 1.095.81 2.22 0 1.605-.015 2.895-.015 3.3 0 .315.225.69.825.57A12.02 12.02 0 0024 12c0-6.63-5.37-12-12-12z"/></svg>
                            GitHub
                        </a>
                    )}
                    <div
                        className="flex items-center gap-2 text-xs px-2.5 py-1 rounded-lg"
                        style={{
                            background: 'rgba(16,185,129,0.08)',
                            border: '1px solid rgba(16,185,129,0.2)',
                        }}
                    >
                        <Activity size={11} className="text-emerald-400 animate-pulse" />
                        <span className="text-emerald-400/80 hidden sm:inline font-medium">Online</span>
                    </div>
                </div>

            </div>
        </header>
    )
}
