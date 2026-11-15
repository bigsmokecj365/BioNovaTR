@echo off
cd /d "%~dp0"
python bionova_aktarici.py
if errorlevel 9009 py bionova_aktarici.py
pause
