#!/bin/bash
# build.sh - Build and Install Script for ifnoon VCV Rack Plugin Collection
# 
# Copyright (c) 2025 ifnoon
# 
# This script automates the build, installation, and testing process for
# the ifnoon plugin collection including Comparally and Matho modules. 
# It handles:
# - Clean build process
# - Installation to VCV Rack user directories
# - Restarting VCV Rack for testing
# 
# License: GPL-3.0-or-later
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

set -e  # keep simple; we avoid set -u so empty vars don't hard-fail

# --- CONFIG: edit if your paths differ ---
RACK_DIR="${RACK_DIR:-../../Rack-SDK}"
RACK2_USER_DIR="$HOME/Library/Application Support/Rack2/plugins-mac-arm64"
RACK2PRO_USER_DIR="$HOME/Library/Application Support/Rack2Pro/plugins-mac-arm64"
RACK_APP_BUNDLE="${RACK_APP_BUNDLE:-/Applications/VCV Rack 2 Free.app}"
RACK_APP_NAME="${RACK_APP_NAME:-VCV Rack 2 Free}"   # shown in macOS
RESTART_DELAY="${RESTART_DELAY:-1}"                  # seconds to wait before relaunch
# ----------------------------------------

# Pretty print helpers
b() { printf "\033[1m%s\033[0m\n" "$*"; }
g() { printf "\033[32m%s\033[0m\n" "$*"; }
r() { printf "\033[31m%s\033[0m\n" "$*"; }
y() { printf "\033[33m%s\033[0m\n" "$*"; }

# Basic checks
[[ -f "Makefile" && -f "plugin.json" ]] || { r "Run this in your plugin folder (needs Makefile & plugin.json)"; exit 1; }
[[ -f "$RACK_DIR/plugin.mk" ]] || { r "Can't find SDK plugin.mk at: $RACK_DIR"; echo 'Set env: export RACK_DIR="/full/path/to/Rack-SDK"'; exit 1; }

# Parse brand name without jq (VCV Rack expects directory name to match brand)
PLUGIN_BRAND="$(sed -nE 's/.*"brand"[[:space:]]*:[[:space:]]*"([^"]+)".*/\1/p' plugin.json | head -n1)"
[[ -n "$PLUGIN_BRAND" ]] || { r 'Could not read "brand" from plugin.json'; exit 1; }

# Pick destination (prefer Rack2)
DEST_BASE="$RACK2_USER_DIR"
[[ -d "$RACK2_USER_DIR" ]] || { y "Using Rack2Pro plugins folder"; DEST_BASE="$RACK2PRO_USER_DIR"; }
DEST_DIR="$DEST_BASE/$PLUGIN_BRAND"

b "→ Building \"$PLUGIN_BRAND\" plugin collection with SDK at: $RACK_DIR"
export RACK_DIR

b "→ make clean"
make clean || true
b "→ make"
make

[[ -f "plugin.dylib" ]] || { r "Build finished but plugin.dylib not found"; exit 1; }

b "→ Installing to: $DEST_DIR"
mkdir -p "$DEST_DIR"
cp -f plugin.json plugin.dylib "$DEST_DIR/"
[[ -d "res" ]] && rsync -a --delete res "$DEST_DIR/"

# Arch info (should be arm64)
file plugin.dylib || true

g "✔ Build & install complete."

# -------- Graceful stop & reliable relaunch of Rack --------
if [[ "${1-}" != "--no-restart" ]]; then
  b "→ Stopping $RACK_APP_NAME (gracefully if running)"

  # Ask the app to quit nicely (avoids crash-recovery dialog)
  /usr/bin/osascript <<OSASCRIPT >/dev/null 2>&1 || true
  tell application "System Events"
    if exists process "$RACK_APP_NAME" then
      tell application "$RACK_APP_NAME" to quit
    end if
  end tell
OSASCRIPT

  # Helper: check if Rack is running (binary name is usually "Rack")
  is_running() {
    pgrep -x Rack >/dev/null && return 0
    pgrep -f "$RACK_APP_NAME" >/dev/null && return 0
    return 1
  }

  # Wait up to ~2s for clean exit, then escalate if needed
  for i in {1..10}; do
    is_running || break
    sleep 0.2
  done
  if is_running; then
    y "Rack still running; sending SIGTERM…"
    pkill -x Rack || pkill -f "$RACK_APP_NAME" || true
    sleep 0.5
  fi
  if is_running; then
    y "Rack still running; forcing SIGKILL…"
    pkill -9 -x Rack || pkill -9 -f "$RACK_APP_NAME" || true
    sleep 0.5
  fi



  # Relaunch
  if [[ -d "$RACK_APP_BUNDLE" ]]; then
    b "→ Relaunching $RACK_APP_NAME"
    # prefer open -a by name (handles bundle properly), fall back to bundle path
    open -a "$RACK_APP_NAME" || open "$RACK_APP_BUNDLE" || true

    # Wait up to ~6s for the app to appear
    launched=0
    for i in {1..30}; do
      if is_running; then launched=1; break; fi
      sleep 0.2
    done

    # Retry with -n (force new instance) if first launch didn't stick
    if [[ $launched -eq 0 ]]; then
      y "First launch didn't appear; retrying with 'open -n'…"
      open -n "$RACK_APP_BUNDLE" || true
      for i in {1..30}; do
        if is_running; then launched=1; break; fi
        sleep 0.2
      done
    fi

    if [[ $launched -eq 1 ]]; then
      g "✓ $RACK_APP_NAME relaunched"
    else
      r "✗ Could not confirm $RACK_APP_NAME started. Launch manually to check errors."
    fi
  else
    r "⚠ Could not find: $RACK_APP_BUNDLE (edit build.sh if moved)."
  fi
else
  y "Skipping restart (flag --no-restart)"
fi

echo
b "Next:"
echo "  • Help → Log: look for \"Loading plugin $PLUGIN_SLUG …\""
echo "  • Module Browser → search your Brand from plugin.json"
echo "  • Look for both Comparally and Matho modules under ifnoon brand"

