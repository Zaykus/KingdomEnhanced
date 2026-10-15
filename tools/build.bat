@echo off
setlocal

set ROOT=%~dp0..
set COMPILER_BIN=%ROOT%\tools\w64devkit\bin
set GXX=%COMPILER_BIN%\g++.exe

if not exist "%GXX%" (
    echo Error: Compiler not found at %GXX%
    echo Run powershell -File tools\setup_compiler.ps1 first!
    exit /b 1
)

:: Ensure MinGW bin directory is in PATH so g++ can locate as, ld, etc.
set PATH=%COMPILER_BIN%;%PATH%

set OUT=%ROOT%\KingdomEnhanced-Standalone\version.dll
set SRC=%ROOT%\KingdomEnhanced-Standalone\src
set THIRD=%ROOT%\KingdomEnhanced-Standalone\third_party
set IMGUI=%THIRD%\imgui
set MINHOOK=%THIRD%\minhook

echo =======================================================
echo Building Kingdom Enhanced (version.dll)
echo =======================================================

"%GXX%" -O2 -DNDEBUG -std=c++17 -shared -o "%OUT%" ^
    "%SRC%\core\proxy.S" ^
    "%SRC%\core\version.def" ^
    "%SRC%\core\dllmain.cpp" ^
    "%SRC%\core\il2cpp.cpp" ^
    "%SRC%\core\config.cpp" ^
    "%SRC%\localization\i18n.cpp" ^
    "%SRC%\ui\overlay.cpp" ^
    "%SRC%\ui\fonts.cpp" ^
    "%SRC%\ui\theme.cpp" ^
    "%SRC%\ui\menu.cpp" ^
    "%SRC%\ui\map.cpp" ^
    "%SRC%\ui\stamina.cpp" ^
    "%SRC%\features\cheats.cpp" ^
    "%SRC%\features\spawner.cpp" ^
    "%SRC%\features\lab.cpp" ^
    "%SRC%\features\hard.cpp" ^
    "%SRC%\features\monitor.cpp" ^
    "%IMGUI%\imgui.cpp" ^
    "%IMGUI%\imgui_draw.cpp" ^
    "%IMGUI%\imgui_tables.cpp" ^
    "%IMGUI%\imgui_widgets.cpp" ^
    "%IMGUI%\backends\imgui_impl_dx11.cpp" ^
    "%IMGUI%\backends\imgui_impl_dx12.cpp" ^
    "%MINHOOK%\src\buffer.c" ^
    "%MINHOOK%\src\hook.c" ^
    "%MINHOOK%\src\trampoline.c" ^
    "%MINHOOK%\src\hde\hde64.c" ^
    -I"%SRC%" ^
    -I"%SRC%\core" ^
    -I"%SRC%\ui" ^
    -I"%SRC%\features" ^
    -I"%SRC%\localization" ^
    -I"%MINHOOK%\include" ^
    -I"%IMGUI%" ^
    -I"%IMGUI%\backends" ^
    -static -static-libgcc -static-libstdc++ -s -Wall -Wno-unused-function -ldxguid -lshlwapi -ld3d11 -ld3d12 -ldxgi

if %ERRORLEVEL% equ 0 (
    echo.
    echo [SUCCESS] built "%OUT%"
    exit /b 0
) else (
    echo.
    echo [FAILURE] Build failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
