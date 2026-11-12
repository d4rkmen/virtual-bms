@echo off
setlocal EnableExtensions
chcp 65001 >nul
set "ROOT=%~dp0"
set "WEB=%ROOT%vbms-web-ui"
set "DIST=%WEB%\dist"
set "TARGET=%ROOT%fs"
set "LOG=%ROOT%BATTERY_HUB_BUILD.log"
cd /d "%ROOT%"
echo BATTERY HUB - local web build
echo Project: %ROOT%
echo Log: %LOG%
if not exist "%WEB%\package.json" (
  echo ERROR: vbms-web-ui\package.json not found
  goto :fail
)
where node >nul 2>&1 || (echo ERROR: Node.js not found & goto :fail)
where npm.cmd >nul 2>&1 || (echo ERROR: npm.cmd not found & goto :fail)
cd /d "%WEB%"
echo Installing dependencies...
call npm.cmd ci > "%LOG%" 2>&1
if errorlevel 1 (
  echo npm ci failed. Try inspecting BATTERY_HUB_BUILD.log
  goto :fail
)
echo Building web UI...
call npm.cmd run build >> "%LOG%" 2>&1
if errorlevel 1 (
  echo Webpack build failed. See BATTERY_HUB_BUILD.log
  goto :fail
)
for %%F in (index.html runtime.js main.js app.js.gz vendor.js.gz app.css.gz vendor.css.gz) do (
  if not exist "%DIST%\%%F" (
    echo ERROR: missing %%F in dist
    goto :fail
  )
)
echo Copying web assets to fs root...
REM Mongoose OS http-server serves / from fs/; index.html references ./app.js.gz.
REM Keep unrelated files in fs; clean only previously generated frontend output.
if not exist "%TARGET%" mkdir "%TARGET%"
if exist "%TARGET%\dist" rmdir /s /q "%TARGET%\dist"
for %%F in (index.html runtime.js main.js app.js app.js.gz vendor.js vendor.js.gz app.css app.css.gz vendor.css vendor.css.gz favicon.ico) do (
  if exist "%TARGET%\%%F" del /q "%TARGET%\%%F"
)
xcopy "%DIST%\*" "%TARGET%\" /E /I /Y >nul
if errorlevel 1 goto :fail
for %%F in (index.html runtime.js main.js app.js.gz vendor.js.gz app.css.gz vendor.css.gz) do (
  if not exist "%TARGET%\%%F" (
    echo ERROR: %%F was not copied to fs root
    goto :fail
  )
)
echo.
echo SUCCESS: web assets ready under fs\
echo Now run: mos build --platform esp32 --local
echo After a successful firmware build, flash only when ready.
pause
exit /b 0
:fail
echo.
echo BUILD FAILED. Log: %LOG%
if exist "%LOG%" powershell -NoProfile -Command "Get-Content -LiteralPath '%LOG%' -Tail 35"
pause
exit /b 1
