@echo off
cd /d "%~dp0"
if not exist "Runtime\SolidArcWorkspace.exe" (echo Extract the Windows runtime bundle here first. & pause & exit /b 1)
"Runtime\SolidArcWorkspace.exe" --options "%~dp0SolidArcWorkspace.toml" --open "%~dp0Liger.solidarc"
