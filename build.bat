@echo off
chcp 65001 >nul

if "%1"=="" goto build
if "%1"=="-f" goto build_and_flash
if "%1"=="/f" goto build_and_flash

:build
echo ====== 仅编译 ======
"D:\Software\Programme\MCU\Keil_v5\UV4\UV4.exe" -b "smart_medicine_box.uvprojx"
goto check_result

:build_and_flash
echo ====== 编译 + 烧录 ======
"D:\Software\Programme\MCU\Keil_v5\UV4\UV4.exe" -f "smart_medicine_box.uvprojx"
goto check_result

:check_result
echo.
if %errorlevel%==0 (
    echo ✓ 成功 (无错误)
) else if %errorlevel%==1 (
    echo ✓ 完成 (有警告)
) else if %errorlevel%==2 (
    echo ✗ 失败 (有错误)
) else (
    echo ✗ 未知错误, errorlevel=%errorlevel%
)
echo.
pause
