cmake_minimum_required(VERSION 3.20)
project(otelapicpp VERSION 1.0.15 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Crow CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(opentelemetry-cpp CONFIG REQUIRED)

add_executable(otelapicpp
  src/api.cpp
  src/config.cpp
  src/main.cpp
  src/telemetry.cpp
  src/weather.cpp
)

target_include_directories(otelapicpp PRIVATE include)

function(link_first_existing_target target)
  set(options)
  set(oneValueArgs REQUIRED_LABEL)
  set(multiValueArgs CANDIDATES)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  foreach(candidate IN LISTS ARG_CANDIDATES)
    if(TARGET ${candidate})
      target_link_libraries(${target} PRIVATE ${candidate})
      message(STATUS "Linked ${target} with ${candidate}")
      return()
    endif()
  endforeach()

  message(FATAL_ERROR "Could not find an OpenTelemetry target for ${ARG_REQUIRED_LABEL}. Candidates: ${ARG_CANDIDATES}")
endfunction()

target_link_libraries(otelapicpp PRIVATE
  Crow::Crow
  nlohmann_json::nlohmann_json
)

# Core SDK/API targets exposed by vcpkg's opentelemetry-cpp package.
link_first_existing_target(otelapicpp
  REQUIRED_LABEL "OpenTelemetry API"
  CANDIDATES
    opentelemetry-cpp::api
    opentelemetry::api
)

link_first_existing_target(otelapicpp
  REQUIRED_LABEL "OpenTelemetry SDK"
  CANDIDATES
    opentelemetry-cpp::sdk
    opentelemetry::sdk
)

link_first_existing_target(otelapicpp
  REQUIRED_LABEL "OTLP HTTP exporter"
  CANDIDATES
    opentelemetry-cpp::otlp_http_exporter
    opentelemetry-cpp::opentelemetry_exporter_otlp_http
    opentelemetry::exporter_otlp_http
)

link_first_existing_target(otelapicpp
  REQUIRED_LABEL "OTLP HTTP client"
  CANDIDATES
    opentelemetry-cpp::otlp_http_client
    opentelemetry-cpp::opentelemetry_exporter_otlp_http_client
    opentelemetry::exporter_otlp_http_client
)

link_first_existing_target(otelapicpp
  REQUIRED_LABEL "OpenTelemetry Logs SDK"
  CANDIDATES
    opentelemetry-cpp::logs
    opentelemetry::logs
)

link_first_existing_target(otelapicpp
  REQUIRED_LABEL "OTLP HTTP log exporter"
  CANDIDATES
    opentelemetry-cpp::otlp_http_log_record_exporter
    opentelemetry-cpp::opentelemetry_exporter_otlp_http_log
    opentelemetry::exporter_otlp_http_log
)

set(OTLP_HTTP_METRIC_EXPORTER_CANDIDATES
  opentelemetry-cpp::otlp_http_metric_exporter
  opentelemetry-cpp::opentelemetry_exporter_otlp_http_metric
  opentelemetry::exporter_otlp_http_metric
)

set(otlp_metric_exporter_linked FALSE)
foreach(candidate IN LISTS OTLP_HTTP_METRIC_EXPORTER_CANDIDATES)
  if(TARGET ${candidate})
    target_link_libraries(otelapicpp PRIVATE ${candidate})
    message(STATUS "Linked otelapicpp with ${candidate}")
    set(otlp_metric_exporter_linked TRUE)
    break()
  endif()
endforeach()

if(NOT otlp_metric_exporter_linked)
  message(WARNING "OTLP HTTP metric exporter target not found. API metric export may be disabled.")
endif()
