#!/bin/bash
# Stage 13 — build the final product end-to-end.
# Rebuilds the binary, installs it as the root ./AgentBasedModeling (which the
# .app launcher runs), and bumps the .app bundle timestamp so Finder shows it as
# the newest version. Run this after ANY change to src/.
set -e
ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$ROOT"

cmake --build build
cp build/AgentBasedModeling ./AgentBasedModeling

# Ad-hoc re-sign: a freshly rebuilt binary has no valid signature, so macOS
# kills it with SIGKILL (exit 137) when launched from Finder. Sign it so the
# double-clicked .app runs.
codesign --force --sign - ./AgentBasedModeling

# Bump bundle mtime so the user sees a fresh timestamp in Finder.
touch AgentBasedModeling.app AgentBasedModeling.app/Contents AgentBasedModeling.app/Contents/MacOS

echo "Built newest version:"
ls -la AgentBasedModeling AgentBasedModeling.app
