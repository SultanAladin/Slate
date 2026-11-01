#!/bin/sh
set -eu
cd "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
exec ./Runtime/SolidArcWorkspace --options "$PWD/SolidArcWorkspace.toml" --open "$PWD/Liger.solidarc"
