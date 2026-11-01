@echo off
cd /d "%~dp0.."
"Runtime\SolidArcWorkspace.exe" --options "Views\Front.toml" --open "Liger.solidarc"
