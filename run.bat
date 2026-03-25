@echo off
cd /d "%~dp0"
set "project_root=%cd%"
.\build\Debug\animations.exe
pause
