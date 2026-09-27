import { useState } from 'react'
import Header from './components/Header'
import IntroPage from './components/IntroPage'
import UploadPage from './components/UploadPage'
import ProcessingPage from './components/ProcessingPage'
import ResultsPage from './components/ResultsPage'

// State-machine router: intro → upload → processing → results
export default function App() {
  const [page, setPage] = useState('intro')
  const [jobId, setJobId] = useState(null)
  const [settings, setSettings] = useState({
    conf: 0.4,
    ego_speed: 60,
    device: 'cpu',
    max_frames: 0,
  })

  function handleStart() {
    setPage('upload')
  }

  function handleJobCreated(id) {
    setJobId(id)
    setPage('processing')
  }

  function handleDone() {
    setPage('results')
  }

  function handleReset() {
    setJobId(null)
    setPage('upload')
  }

  function handleHome() {
    setJobId(null)
    setPage('intro')
  }

  return (
    <div className="min-h-screen flex flex-col">
      <Header page={page} onHome={handleHome} />

      <main className="flex-1 max-w-7xl mx-auto w-full px-4 sm:px-6 lg:px-8 py-8">
        {page === 'intro' && (
          <IntroPage onStart={handleStart} />
        )}
        {page === 'upload' && (
          <UploadPage
            settings={settings}
            setSettings={setSettings}
            onJob={handleJobCreated}
          />
        )}
        {page === 'processing' && jobId && (
          <ProcessingPage jobId={jobId} onDone={handleDone} />
        )}
        {page === 'results' && jobId && (
          <ResultsPage jobId={jobId} onReset={handleReset} />
        )}
      </main>

      {/* Footer */}
      <footer
        className="py-5 text-center text-xs space-y-1"
        style={{ borderTop: '1px solid rgba(255,255,255,0.04)' }}
      >
        <p className="text-gray-600">ADAS Perception System · YOLOv11n · IoU Tracking · FCW</p>
        <p className="text-gray-700">Built with React + FastAPI · © 2025 Om Jagdale</p>
      </footer>
    </div>
  )
}
