#!/usr/bin/env bash
set -euo pipefail
if command -v python3 >/dev/null 2>&1 && command -v gcc >/dev/null 2>&1; then
  echo "Python 3 and GCC are available. No installation required."
  exit 0
fi
if ! command -v apt-get >/dev/null 2>&1; then
  echo "Install Python 3 and GCC using your Linux distribution's package manager, then run start.sh again."
  exit 1
fi
if [[ "$EUID" -eq 0 ]]; then
  apt-get update
  apt-get install -y python3 build-essential
else
  sudo apt-get update
  sudo apt-get install -y python3 build-essential
fi
