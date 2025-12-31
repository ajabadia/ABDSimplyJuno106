@echo off
setlocal

echo ========================================
echo ABDSimpleJuno106 Build Script (Standalone Only)
echo ========================================
echo.

set BUILD_DIR=build
set GENERATOR="Visual Studio 18 2026"
set CMAKE_PATH="C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

if not exist %CMAKE_PATH% (
    echo [ERROR] CMake not found at %CMAKE_PATH%
    echo Attempting to use cmake from PATH...
    set CMAKE_PATH=cmake
)

echo 1. Configurando proyecto con CMake...
if not exist %BUILD_DIR% mkdir %BUILD_DIR%

%CMAKE_PATH% -B %BUILD_DIR% -G %GENERATOR% -A x64

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] CMake configuration failed.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo 2. Compilando ABDSimpleJuno106 Standalone (Release)...
%CMAKE_PATH% --build %BUILD_DIR% --config Release --target ABDSimpleJuno106_Standalone

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo 3. Buscando ejecutable...
set EXE_SRC=%BUILD_DIR%\ABDSimpleJuno106_artefacts\Release\Standalone\ABDSimpleJuno106.exe
if not exist "%EXE_SRC%" set EXE_SRC=%BUILD_DIR%\Release\ABDSimpleJuno106.exe

if exist "%EXE_SRC%" (
    copy /Y "%EXE_SRC%" "ABDSimpleJuno106.exe"
    echo    Ejecutable copiado a la raiz: ABDSimpleJuno106.exe
) else (
    echo    ADVERTENCIA: No se encontro el ejecutable en las rutas esperadas.
)

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo Quieres ejecutarlo ahora? [S,N]?
set /p RESPUESTA=
if /i "%RESPUESTA%"=="S" (
    start ABDSimpleJuno106.exe
)
pause
