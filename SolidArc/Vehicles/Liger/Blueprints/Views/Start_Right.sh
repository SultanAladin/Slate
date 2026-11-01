#!/bin/sh
set -eu
cd "$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec ./Runtime/SolidArcWorkspace --options "Views/Right.toml" --open "$PWD/Liger.solidarc"
