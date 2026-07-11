import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      '/otlp': {
        target: 'http://localhost:4318',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path.replace(/^\/otlp/, ''),
      },
    },
  },
})
