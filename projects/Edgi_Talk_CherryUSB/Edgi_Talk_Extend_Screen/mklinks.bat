@echo off
cd /d "%~dp0"
@echo on
mklink /J rt-thread ..\..\..\rt-thread
mklink /J libraries ..\..\..\libraries
mklink /J libs ..\..\libs
mklink /J tools ..\..\..\tools
mklink /J resources ..\..\..\resources