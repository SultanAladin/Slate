#!/usr/bin/env bash
#====================================================================================================================================
#                                                    CROSSBUILDWINDOWS.SH
#====================================================================================================================================
# 🧩 Windows x64 cross-build of SolidArc inside an overlaid Frontier checkout, then the preview package (zip)
#    bash Experimental/SolidArc/Native/Tooling/CrossBuildWindows.sh <Frontier checkout> [build folder] [targets…]
#    Default targets: SolidArc (SolidArcWorkspace.exe) and Frontier (Frontier.exe with the SolidArc browser link).
#    Compiler: Zig (pip package ziglang) → clang for x86_64-windows-gnu with MinGW-w64; tools come from a venv under _ClaudeScratch/.
set -euo pipefail
cd "$(dirname "$0")/../../../.."
RepositoryRoot="$PWD"
Frontier="$(cd "${1:?Frontier checkout}" && pwd)"
Build="${2:-$RepositoryRoot/_ClaudeScratch/build/Windows}"
Build="$(realpath -m "$Build")"
shift $(( $# > 2 ? 2 : $# ))
Targets="${*:-SolidArc Frontier}"

#------------------------------------------------------------------------------------------------------------------------
#                                                    TOOLS
#------------------------------------------------------------------------------------------------------------------------
Venv="$RepositoryRoot/_ClaudeScratch/.venv"
ZigVenv="$RepositoryRoot/_ClaudeScratch/ZigVenv"
Tools="$RepositoryRoot/_ClaudeScratch/tmp/ZigTools"
[ -x "$Venv/bin/ninja" ] || { python3 -m venv "$Venv" && "$Venv/bin/pip" install -q cmake ninja; }
[ -x "$ZigVenv/bin/python" ] && "$ZigVenv/bin/python" -c "import ziglang" 2>/dev/null || { python3 -m venv "$ZigVenv" && "$ZigVenv/bin/pip" install -q ziglang; }
export PATH="$Venv/bin:$PATH"
mkdir -p "$Tools"
for Tool in cc c++ ar ranlib rc; do
    Target=""
    [[ "$Tool" == cc || "$Tool" == c++ ]] && Target="-target x86_64-windows-gnu"
    printf '#!/bin/sh\nexport ZIG_GLOBAL_CACHE_DIR="%s"\nexec "%s" -m ziglang %s %s "$@"\n' \
        "$RepositoryRoot/_ClaudeScratch/tmp/ZigCache" "$ZigVenv/bin/python" "$Tool" "$Target" > "$Tools/zig-$Tool"
    chmod +x "$Tools/zig-$Tool"
done

#------------------------------------------------------------------------------------------------------------------------
#                                                    CONFIGURE AND BUILD
#------------------------------------------------------------------------------------------------------------------------
Python="${SOLIDARC_FONT_PYTHON:-$RepositoryRoot/_ClaudeScratch/FontVenv/bin/python}"
[ -x "$Python" ] || Python="$RepositoryRoot/_ClaudeScratch/.venv/bin/python"                   # Font tools also live in the build venv
cmake -S "$Frontier" -B "$Build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE="$RepositoryRoot/Experimental/SolidArc/Native/Tooling/WindowsZigToolchain.cmake" \
      -DSOLIDARC_ZIG_TOOLS="$Tools" -DPython3_EXECUTABLE="$Python" -DBUILD_TESTING=OFF \
      -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF ${SOLIDARC_CMAKE_EXTRA:-}
ninja -C "$Build" -j"${SOLIDARC_JOBS:-2}" $Targets

#------------------------------------------------------------------------------------------------------------------------
#                                                    PACKAGE
#------------------------------------------------------------------------------------------------------------------------
rm -rf "$Build/Package/SolidArc"
cmake --install "$Build" --component SolidArc --prefix "$Build/Package/SolidArc"
python3 - "$Build/Package" <<'PY'
import os, sys, zipfile
pkg_dir = sys.argv[1]
root_dir = os.path.join(pkg_dir, "SolidArc")
zip_path = os.path.join(pkg_dir, "SolidArc-Windows-x64.zip")
if os.path.exists(zip_path):
    os.remove(zip_path)
solidarc_svgs = 0
with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
    for dirpath, _, filenames in os.walk(root_dir, followlinks=True):
        for fname in sorted(filenames):
            full = os.path.join(dirpath, fname)
            rel = os.path.relpath(full, pkg_dir).replace(os.sep, "/")
            if "EngineContent/Icons/SolidArc/" in rel and rel.endswith(".svg"):
                solidarc_svgs += 1
            zf.write(os.path.realpath(full), rel)
if solidarc_svgs < 60:
    raise SystemExit(f"CrossBuildWindows: missing SolidArc icons in zip (found {solidarc_svgs})")
print(f"CrossBuildWindows: verified {solidarc_svgs} SolidArc SVG icons in {zip_path}")
PY
echo "CrossBuildWindows: $Build/Package/SolidArc-Windows-x64.zip"
