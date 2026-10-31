#!/usr/bin/env bash
set -euo pipefail
PROJECT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"
if [[ "$(uname -s)" != "Linux" ]]; then
  echo "PipeTalk requires Linux. On Windows, use START_WINDOWS.bat with Ubuntu on WSL."
  exit 1
fi
if ! command -v python3 >/dev/null 2>&1 || ! command -v gcc >/dev/null 2>&1; then
  echo "Installing the missing Ubuntu prerequisites (Python 3 and GCC)."
  echo "Ubuntu may ask for your sudo password. Password characters are not displayed."
  bash "$PROJECT_DIR/scripts/setup_ubuntu.sh"
fi
if [[ "${PIPETALK_NO_BROWSER:-0}" == "1" ]]; then
  exec python3 "$PROJECT_DIR/backend/server.py" "$@"
else
  exec python3 "$PROJECT_DIR/backend/server.py" --open-browser "$@"
fi
