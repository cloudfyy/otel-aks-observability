#!/usr/bin/env bash
set -euo pipefail

acr_login_server="${ACR_LOGIN_SERVER:-}"
image_tag=""
otel_environment_name=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --acr-login-server)
      acr_login_server="$2"
      shift 2
      ;;
    --image-tag)
      image_tag="$2"
      shift 2
      ;;
    --otel-environment-name)
      otel_environment_name="$2"
      shift 2
      ;;
    -h|--help)
      cat <<'EOF'
Usage: build-push-acr.sh --acr-login-server <ACR_LOGIN_SERVER> --image-tag <IMAGE_TAG> [--otel-environment-name <ENV_NAME>]
EOF
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 2
      ;;
  esac
done

if [[ -z "$acr_login_server" ]]; then
  echo "Missing --acr-login-server or ACR_LOGIN_SERVER." >&2
  exit 1
fi

if [[ -z "$image_tag" ]]; then
  echo "Missing --image-tag." >&2
  exit 1
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
acr_name="${acr_login_server%%.*}"
image="otel-ui:${image_tag}"

echo "Building and pushing image ${acr_login_server}/${image} with az acr build"
build_args=(
  --registry "$acr_name"
  --source-acr-auth-id "[caller]"
  --image "$image"
  --file "$script_dir/Dockerfile"
)

if [[ -n "$otel_environment_name" ]]; then
  build_args+=(--build-arg "VITE_OTEL_ENVIRONMENT_NAME=${otel_environment_name}")
fi

build_args+=("$script_dir")
az acr build "${build_args[@]}"

echo "Done: ${acr_login_server}/${image}"