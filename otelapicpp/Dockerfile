ARG DEPS_IMAGE=otelapicpp-deps:latest
FROM ${DEPS_IMAGE} AS build

ENV DEBIAN_FRONTEND=noninteractive
ENV VCPKG_ROOT=/opt/vcpkg

WORKDIR /src
COPY . .

RUN cmake -S . -B /build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DVCPKG_MANIFEST_MODE=OFF \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
RUN cmake --build /build --target otelapicpp

FROM ubuntu:24.04 AS final

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libcurl4 \
    libprotobuf-dev \
  && rm -rf /var/lib/apt/lists/*

RUN useradd --system --uid 10001 --no-create-home appuser

WORKDIR /app
COPY --from=build /build/otelapicpp /app/otelapicpp

USER 10001

EXPOSE 8082

ENV PORT=8082
ENV OTEL_SERVICE_NAME=otelapidemo-cpp
ENV OTEL_EXPORTER_OTLP_ENDPOINT=http://otel-collector-opentelemetry-collector.observability.svc.cluster.local:4318

ENTRYPOINT ["/app/otelapicpp"]
