@echo off
setlocal EnableExtensions

set "ROOT_DIR=%~dp0.."
for %%I in ("%ROOT_DIR%") do set "ROOT_DIR=%%~fI"
set "VCPKG_DIR=%ROOT_DIR%\vcpkg"
set "RUN_VERIFY=0"
set "BUILD_DIR=build-sdl3-bgfx-jolt-rmlui-sdl3audio"

if /I "%~1"=="-h" goto :help
if /I "%~1"=="--help" goto :help
if /I "%~1"=="--verify" (
  set "RUN_VERIFY=1"
  if not "%~2"=="" set "BUILD_DIR=%~2"
) else if not "%~1"=="" (
  echo [setup] ERROR: unknown argument '%~1'
  goto :usage_error
)

pushd "%ROOT_DIR%" || exit /b 1

where git >nul 2>nul
if errorlevel 1 (
  echo [setup] ERROR: git is required to bootstrap vcpkg.
  popd
  exit /b 1
)

if not exist "%VCPKG_DIR%\" (
  echo [setup] vcpkg\ not found; cloning...
  git clone https://github.com/microsoft/vcpkg.git "%VCPKG_DIR%"
  if errorlevel 1 (
    echo [setup] ERROR: failed to clone vcpkg.
    popd
    exit /b 1
  )
)

if not exist "%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake" (
  echo [setup] ERROR: %VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake missing.
  echo [setup] Recreate .\vcpkg and rerun setup.
  popd
  exit /b 1
)

if not exist "%VCPKG_DIR%\vcpkg.exe" (
  echo [setup] Bootstrapping vcpkg...
  call "%VCPKG_DIR%\bootstrap-vcpkg.bat" -disableMetrics
  if errorlevel 1 (
    echo [setup] ERROR: failed to bootstrap vcpkg.
    popd
    exit /b 1
  )
)

echo [setup] Local vcpkg ready at %VCPKG_DIR%

if "%RUN_VERIFY%"=="1" (
  where py >nul 2>nul
  if errorlevel 1 (
    set "PYTHON_CMD=python"
  ) else (
    set "PYTHON_CMD=py -3"
  )
  echo [setup] Verifying build with .\abuild.py -c %BUILD_DIR%
  %PYTHON_CMD% abuild.py -c %BUILD_DIR%
  if errorlevel 1 (
    echo [setup] ERROR: verify build failed.
    popd
    exit /b 1
  )
)

echo [setup] Done.
popd
exit /b 0

:help
echo Usage: scripts\setup.bat [--verify [build-dir]]
echo.
echo Bootstraps mandatory local .\vcpkg for m-rewrite.
echo.
echo Options:
echo   --verify [build-dir]   After bootstrap, run .\abuild.py -c ^<build-dir^>.
echo                           Default build-dir: build-sdl3-bgfx-jolt-rmlui-sdl3audio
echo   -h, --help             Show this help.
exit /b 0

:usage_error
echo Usage: scripts\setup.bat [--verify [build-dir]]
exit /b 1
