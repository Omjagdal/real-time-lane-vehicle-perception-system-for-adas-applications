const colorMap = {
    cyan: {
        gradient: 'linear-gradient(135deg, rgba(6,182,212,0.12), rgba(6,182,212,0.04))',
        border: 'rgba(6,182,212,0.2)',
        icon: 'rgba(6,182,212,0.15)',
        iconBorder: 'rgba(6,182,212,0.25)',
        text: '#22d3ee',
        glow: '0 4px 16px rgba(6,182,212,0.08)',
    },
    indigo: {
        gradient: 'linear-gradient(135deg, rgba(99,102,241,0.12), rgba(99,102,241,0.04))',
        border: 'rgba(99,102,241,0.2)',
        icon: 'rgba(99,102,241,0.15)',
        iconBorder: 'rgba(99,102,241,0.25)',
        text: '#818cf8',
        glow: '0 4px 16px rgba(99,102,241,0.08)',
    },
    red: {
        gradient: 'linear-gradient(135deg, rgba(239,68,68,0.12), rgba(239,68,68,0.04))',
        border: 'rgba(239,68,68,0.2)',
        icon: 'rgba(239,68,68,0.15)',
        iconBorder: 'rgba(239,68,68,0.25)',
        text: '#f87171',
        glow: '0 4px 16px rgba(239,68,68,0.08)',
    },
    amber: {
        gradient: 'linear-gradient(135deg, rgba(245,158,11,0.12), rgba(245,158,11,0.04))',
        border: 'rgba(245,158,11,0.2)',
        icon: 'rgba(245,158,11,0.15)',
        iconBorder: 'rgba(245,158,11,0.25)',
        text: '#fbbf24',
        glow: '0 4px 16px rgba(245,158,11,0.08)',
    },
}

export default function MetricCard({ icon, label, value, color = 'cyan', unit = '' }) {
    const c = colorMap[color] || colorMap.cyan
    return (
        <div
            className="p-4 space-y-3 animate-slide-up rounded-2xl transition-all duration-300 hover:translate-y-[-2px]"
            style={{
                background: c.gradient,
                border: `1px solid ${c.border}`,
                backdropFilter: 'blur(20px)',
                boxShadow: `${c.glow}, 0 4px 24px rgba(0,0,0,0.2), 0 1px 0 rgba(255,255,255,0.04) inset`,
            }}
        >
            <div
                className="w-8 h-8 rounded-lg flex items-center justify-center"
                style={{
                    background: c.icon,
                    border: `1px solid ${c.iconBorder}`,
                    color: c.text,
                    boxShadow: `0 2px 8px rgba(0,0,0,0.2)`,
                }}
            >
                {icon}
            </div>
            <div>
                <p className="text-2xl font-bold text-white font-mono leading-none">
                    {typeof value === 'number' ? value.toLocaleString() : value}
                    {unit && <span className="text-sm font-normal text-gray-500 ml-1">{unit}</span>}
                </p>
                <p className="text-xs text-gray-500 mt-1.5">{label}</p>
            </div>
        </div>
    )
}
