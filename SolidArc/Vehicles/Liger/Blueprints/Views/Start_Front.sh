#!/bin/sh
set -eu
cd "$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
exec ./Runtime/SolidArcWorkspace --options "Views/Front.toml" --open "$PWD/Liger.solidarc"
