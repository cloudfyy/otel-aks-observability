#!/usr/bin/env bash
set -euo pipefail

acr_login_server="${ACR_LOGIN_SERVER:-}"
image_tag="1.0.1"

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
    -h|--help)
      cat <<'EOF'
Usage: build-push-acr.sh --acr-login-server <ACR_LOGIN_SERVER> [--image-tag 1.0.1]
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

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_context="$(dirname "$script_dir")"
acr_name="${acr_login_server%%.*}"
image="otelapidemo:${image_tag}"

echo "Building and pushing image ${acr_login_server}/${image} with az acr build"
az acr build --registry "$acr_name" --source-acr-auth-id "[caller]" --image "$image" --file "$script_dir/Dockerfile" "$build_context"

echo "Done: ${acr_login_server}/${image}"
