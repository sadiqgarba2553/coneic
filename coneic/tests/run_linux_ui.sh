#!/usr/bin/env bash
# Opt-in native Qt acceptance checks in an isolated X server and profile.
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail
[[ "$(id -u)" != 0 ]] || { echo 'Run the media player tests as a regular user' >&2; exit 1; }
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
output="${1:?Provide a writable test-output directory}"
binary="${2:-/usr/bin/coneic}"
mkdir -p "$output"
output="$(cd "$output" && pwd)"
export CONEIC_TEST_OUTPUT="$output"
export CONEIC_TEST_MEDIA="$root/coneic/tests/fixtures"
export QT_SCALE_FACTOR="${QT_SCALE_FACTOR:-1}"
export XDG_CONFIG_HOME="$output/xdg-config"
export XDG_DATA_HOME="$output/xdg-data"
export XDG_CACHE_HOME="$output/xdg-cache"
export XDG_RUNTIME_DIR="$output/runtime"
mkdir -p "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"
xvfb-run -a -s '-screen 0 3840x2160x24' timeout 100s "$binary" \
    --ignore-config --no-media-library --no-one-instance \
    --no-qt-privacy-ask \
    --avcodec-hw=none --vout=xcb_x11 --aout=dummy \
    >"$output/native-ui-stdout.log" 2>"$output/native-ui-stderr.log"
python3 - "$output" <<'PY'
import json, pathlib, sys
output = pathlib.Path(sys.argv[1])
reports = ['native-ui-results.json', 'personal-persistence-results.json',
           'personal-runtime-results.json', 'library-tools-results.json']
ok = True
for name in reports:
    value = json.loads((output / name).read_text(encoding='utf-8'))
    checks = value['checks']
    passed = value['passed'] and all(c['passed'] for c in checks)
    print(json.dumps({'report': name, 'passed': passed, 'checks': len(checks),
                      'failed': [c for c in checks if not c['passed']]}))
    ok &= passed
sys.exit(0 if ok else 1)
PY
