"""Write portable reference-only project, view options and launchers."""
import json
from pathlib import Path

def project_doc(rows, visible, prefix=''):
    text='Format = "SolidArc"\nFormatVersion = 1\nTitle = "Liger — scratch modelling reference"\nDescription = "Reference images only; empty editable feature tree. Native views: vehicle front -Y, right +X, up +Z. References rigidly rotated from source CAD. Metres internally."\n\n'
    for row in rows:
        text+='[[ImagePlanes]]\n'
        for key,value in dict(Name=f"{row['name']} — CAD tracing reference",Path=prefix+row['image'],
                              Corner=row['corner'],Right=row['right'],Down=row['down'],
                              Opacity=.8,Visible=row['name'] in visible,Photo=True,Locked=True,FrontOnly=True).items():
            text+=f'{key} = {json.dumps(value,ensure_ascii=False)}\n'
        text+='\n'
    return text.rstrip()+"\n"


OPTIONS = '[Window]\nWidth = 1800\nHeight = 1100\n[Viewport]\nOrthographic = true\nGrid = false\nFieldOfView = 0.7\nInitialView = "Top"\nInitialLayout = "Quad"\nFitImages = true\n[Files]\nDocumentPath = "Liger.solidarc"\nRecoveryPath = "Recovery/Liger-autosave.solidarc"\nAutosaveDelay = 1.0\n[Interface]\nFontSize = 15.0\n'


def setup_project(out, rows):
    (out/'Views').mkdir(parents=True, exist_ok=True)
    (out/'Liger.solidarc').write_text(project_doc(rows, {r['name'] for r in rows}))
    (out/'SolidArcWorkspace.toml').write_text(OPTIONS)
    (out/'Start_Liger.bat').write_text('@echo off\ncd /d "%~dp0"\nif not exist "Runtime\\SolidArcWorkspace.exe" (echo Extract the Windows runtime bundle here first. & pause & exit /b 1)\n"Runtime\\SolidArcWorkspace.exe" --options "%~dp0SolidArcWorkspace.toml" --open "%~dp0Liger.solidarc"\n')
    (out/'Start_Liger.sh').write_text('#!/bin/sh\nset -eu\ncd "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"\nexec ./Runtime/SolidArcWorkspace --options "$PWD/SolidArcWorkspace.toml" --open "$PWD/Liger.solidarc"\n')
    (out/'Start_Liger.sh').chmod(0o755)
    for row in rows:
        name=row['name']
        options=OPTIONS.replace('InitialView = "Top"',f'InitialView = "{row["native_preset"]}"').replace('InitialLayout = "Quad"','InitialLayout = "Single"')
        (out/'Views'/f'{name}.toml').write_text(options)
        (out/'Views'/f'Start_{name}.bat').write_text('@echo off\ncd /d "%~dp0.."\n"Runtime\\SolidArcWorkspace.exe" --options "Views\\Top.toml" --open "Liger.solidarc"\n'.replace('Top',name))
        (out/'Views'/f'Start_{name}.sh').write_text('#!/bin/sh\nset -eu\ncd "$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"\nexec ./Runtime/SolidArcWorkspace --options "Views/Top.toml" --open "$PWD/Liger.solidarc"\n'.replace('Top',name))
        (out/'Views'/f'Start_{name}.sh').chmod(0o755)


if __name__ == '__main__':
    out=Path(__file__).resolve().parents[1]/'Blueprints'
    setup_project(out,json.loads((out/'registration.json').read_text())['views'])
