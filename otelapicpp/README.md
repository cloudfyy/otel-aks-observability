# otelapicpp

C++ Web API that mirrors the existing .NET weather endpoints and emits OpenTelemetry traces.

## Endpoints

- GET /weatherforecast
- GET /weatherforecast/throw-custom-exception
- GET /weatherforecast/throw-and-catch-exception
- GET /health

## Local build

This project uses vcpkg manifest mode for dependencies.

1. Install vcpkg and bootstrap it.
2. Configure CMake with the vcpkg toolchain file.

```bash
cmake -S . -B build -G Ninja \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_TOOLCHAIN_FILE=<VCPKG_ROOT>/scripts/buildsystems/vcpkg.cmake
cmake --build build --target otelapicpp
```

## Local run

```bash
PORT=8082 ./build/otelapicpp
```

## Trace export settings

The app reads these environment variables:

- OTEL_SERVICE_NAME (default: otelapidemo-cpp)
- OTEL_EXPORTER_OTLP_ENDPOINT (default: http://otel-collector-opentelemetry-collector.observability.svc.cluster.local:4318)
- OTEL_RESOURCE_ATTRIBUTES (optional, app uses defaults when unset)
