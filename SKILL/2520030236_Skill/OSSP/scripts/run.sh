#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
make -C "$root"
exec "$root/ossp-shell"
