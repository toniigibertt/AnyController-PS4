@echo off
chcp 65001 >nul
title Compilador OneFamily PS4 en Español

echo ==========================================================
echo    OneFamily PS4 - Compilador del Paquete PKG en Español
echo ==========================================================
echo.
echo Iniciando compilacion con permisos de script...
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_app.ps1"

echo.
echo ==========================================================
echo Presiona cualquier tecla para cerrar esta ventana...
echo ==========================================================
pause >nul
