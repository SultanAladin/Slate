#!/bin/sh
set -eu
cd "$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec ./Runtime/SolidArcWorkspace --options "Views/Rear.toml" --open "$PWD/Liger.solidarc"
