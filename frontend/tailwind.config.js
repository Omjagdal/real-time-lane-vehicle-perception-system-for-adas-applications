/** @type {import('tailwindcss').Config} */
export default {
  content: ["./index.html", "./src/**/*.{js,ts,jsx,tsx}"],
  theme: {
    extend: {
      colors: {
        brand: {
          50:  "#e0f7ff",
          400: "#22d3ee",
          500: "#06b6d4",
          600: "#0891b2",
          900: "#050a15",
        },
        surface: {
          DEFAULT: "rgba(255,255,255,0.04)",
          hover:   "rgba(255,255,255,0.07)",
          border:  "rgba(255,255,255,0.08)",
        },
      },
      fontFamily: {
        sans: ["Inter", "system-ui", "-apple-system", "sans-serif"],
        mono: ["JetBrains Mono", "monospace"],
      },
      animation: {
        "pulse-slow": "pulse 3s cubic-bezier(0.4,0,0.6,1) infinite",
        "fade-in"   : "fadeIn 0.5s ease-out both",
        "slide-up"  : "slideUp 0.5s ease-out both",
        "slide-down": "slideDown 0.4s ease-out both",
        "scale-in"  : "scaleIn 0.4s ease-out both",
        "float"     : "float 6s ease-in-out infinite",
        "glow-pulse": "glowPulse 3s ease-in-out infinite",
      },
      keyframes: {
        fadeIn: {
          "0%":   { opacity: 0 },
          "100%": { opacity: 1 },
        },
        slideUp: {
          "0%":   { opacity: 0, transform: "translateY(20px)" },
          "100%": { opacity: 1, transform: "translateY(0)" },
        },
        slideDown: {
          "0%":   { opacity: 0, transform: "translateY(-10px)" },
          "100%": { opacity: 1, transform: "translateY(0)" },
        },
        scaleIn: {
          "0%":   { opacity: 0, transform: "scale(0.95)" },
          "100%": { opacity: 1, transform: "scale(1)" },
        },
        float: {
          "0%, 100%": { transform: "translateY(0px)" },
          "50%":      { transform: "translateY(-12px)" },
        },
        glowPulse: {
          "0%, 100%": { opacity: 0.5 },
          "50%":      { opacity: 1 },
        },
      },
      backdropBlur: {
        xs: "2px",
      },
    },
  },
  plugins: [],
}
