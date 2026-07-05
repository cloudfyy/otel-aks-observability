#!/usr/bin/env bash
set -euo pipefail

namespace="apps-prod"
acr_login_server="${ACR_LOGIN_SERVER:-}"

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Keep the real ACR login server out of committed manifests. The value can come
# from an explicit argument, the environment, or the ignored local env file.
while [[ $# -gt 0 ]]; do
  case "$1" in
    -n|--namespace)
      namespace="$2"
      shift 2
      ;;
    --acr-login-server)
      acr_login_server="$2"
      shift 2
      ;;
    -h|--help)
      cat <<'EOF'
Usage: deploy-apps.sh [--namespace apps-prod] [--acr-login-server <ACR_LOGIN_SERVER>]

Reads ACR_LOGIN_SERVER from, in order:
  1. --acr-login-server
  2. ACR_LOGIN_SERVER environment variable
  3. prod/apps/.env.local
EOF
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 2
      ;;
  esac
done

local_env_path="$script_dir/.env.local"
if [[ -z "$acr_login_server" && -f "$local_env_path" ]]; then
  # Handle files with or without a trailing newline, and strip CR for Windows-edited files.
  while IFS= read -r line || [[ -n "$line" ]]; do
    line="${line%$'\r'}"
    if [[ "$line" == ACR_LOGIN_SERVER=* ]]; then
      acr_login_server="${line#ACR_LOGIN_SERVER=}"
    fi
  done < "$local_env_path"
fi

if [[ -z "$acr_login_server" ]]; then
  echo "Set ACR_LOGIN_SERVER, pass --acr-login-server, or create prod/apps/.env.local from .env.example." >&2
  exit 1
fi

# Render the shared Kustomize base, then replace the image placeholder only in
# the stream sent to kubectl. The repository files keep <ACR_LOGIN_SERVER>.
kubectl kustomize "$script_dir" |
  sed "s|<ACR_LOGIN_SERVER>|$acr_login_server|g" |
  kubectl apply -n "$namespace" -f -