#!/usr/bin/env bash
set -euo pipefail

acr_login_server="${ACR_LOGIN_SERVER:-}"
image_tag=""
skip_deps="false"

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
    --skip-deps)
      skip_deps="true"
      shift 1
      ;;
    -h|--help)
      cat <<'EOF'
    Usage: build-push-acr.sh --acr-login-server <ACR_LOGIN_SERVER> --image-tag <IMAGE_TAG> [--skip-deps]
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
resolved_acr_login_server="$(az acr show --name "$acr_name" --query loginServer --output tsv)"
if [[ -z "$resolved_acr_login_server" ]]; then
  echo "Unable to resolve ACR login server for registry '$acr_name'." >&2
  exit 1
fi
image="otelapicpp:${image_tag}"
manifest_hash="$(sha256sum "$script_dir/vcpkg.json" | awk '{print tolower(substr($1,1,12))}')"
deps_image="otelapicpp-deps:${manifest_hash}"

print_latest_run() {
  local run
  run="$(az acr task list-runs --registry "$acr_name" --top 1 --output tsv --query '[0].[runId,status,runDuration]')"
  if [[ -n "$run" ]]; then
    echo "Latest run: $run"
  fi
}

get_image_digest() {
  local image_name="$1"
  az acr repository show --name "$acr_name" --image "$image_name" --query digest --output tsv 2>/dev/null || true
}

echo "ACR registry: ${resolved_acr_login_server} (name: ${acr_name})"
echo "Manifest hash (vcpkg.json): ${manifest_hash}"
echo "Deps image candidate: ${resolved_acr_login_server}/${deps_image}"

deps_digest="$(get_image_digest "$deps_image")"
if [[ -n "$deps_digest" ]]; then
  echo "Deps image cache hit: digest=${deps_digest}"
else
  echo "Deps image cache miss: ${deps_image} not found in registry"
fi

if [[ "$skip_deps" == "true" && -z "$deps_digest" ]]; then
  echo "--skip-deps was set, but deps image '${resolved_acr_login_server}/${deps_image}' does not exist." >&2
  exit 1
fi

if [[ "$skip_deps" != "true" && -z "$deps_digest" ]]; then
  echo "Building deps image ${resolved_acr_login_server}/${deps_image} with az acr build"
  deps_start="$(date +%s)"
  az acr build --registry "$acr_name" --source-acr-auth-id "[caller]" --image "$deps_image" --file "$script_dir/Dockerfile.deps" "$script_dir"
  deps_end="$(date +%s)"
  echo "Deps build elapsed: $((deps_end - deps_start))s"
  print_latest_run
  deps_digest="$(get_image_digest "$deps_image")"
  if [[ -n "$deps_digest" ]]; then
    echo "Deps image published: digest=${deps_digest}"
  fi
elif [[ "$skip_deps" != "true" && -n "$deps_digest" ]]; then
  echo "Skipping deps build due to cache hit"
else
  echo "Skipping deps build due to --skip-deps"
fi

echo "Building and pushing image ${resolved_acr_login_server}/${image} with az acr build"
app_start="$(date +%s)"
az acr build --registry "$acr_name" --source-acr-auth-id "[caller]" --image "$image" --build-arg "DEPS_IMAGE=${resolved_acr_login_server}/${deps_image}" --file "$script_dir/Dockerfile" "$script_dir"
app_end="$(date +%s)"
echo "App build elapsed: $((app_end - app_start))s"
print_latest_run

app_digest="$(get_image_digest "$image")"
if [[ -n "$app_digest" ]]; then
  echo "App image digest: ${app_digest}"
fi

echo "Done: ${resolved_acr_login_server}/${image}"
