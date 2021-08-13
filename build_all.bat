
@echo off

setlocal
set SDK_ROOT=./nRF5_SDK_17.0.2_d674dde

del /q "./Release"


"%ProgramFiles%\SEGGER\SEGGER Embedded Studio for ARM 5.50d\bin\emBuild.exe" -config "Release" -echo -show -verbose -time ^
"\application\pca10040\s312\ses\sensor_application_s312.emProject" ".\bootloader\pca10040_s312_ble_debug\ses\secure_bootloader_ble_s312_pca10040_debug.emProject"

call :NORMALIZEPATH %SDK_ROOT%
set SDK_ROOT=%RETVAL%

copy  "%SDK_ROOT%\components\softdevice\s312\hex\*.hex" "./Release/s312.hex"

nrfutil settings generate --family NRF52 --application "./Release/sensor_application_s312/sensor_application_s312.hex"  --application-version 1 --bootloader-version 2 --bl-settings-version 1 "./Release/settings.hex"

nrfutil pkg generate --application "./Release/sensor_application_s312/sensor_application_s312.hex" --application-version 1 ^
--hw-version 1 ^
--sd-req  0xBB ^
"./Release/sensor_application.zip"

nrfutil pkg generate --application "./Release/sensor_application_s312/sensor_application_s312.hex" --application-version 1 ^
--hw-version 1 ^
--sd-req  0xBB ^
--sd-id  0xBB ^
--softdevice  "./Release/s312.hex" ^
"./Release/sensor_application_with_s312.zip"

mergehex -m "./Release/sensor_application_s312/sensor_application_s312.hex" "./Release/s312.hex" "./Release/settings.hex" ^
"./Release/secure_bootloader_ble_s312_pca10040_debug/secure_bootloader_ble_s312_pca10040_debug.hex" -o "./Release/sensor_application_with_s312_with_bootloader.hex"


:: ========== FUNCTIONS ==========
EXIT /B

:NORMALIZEPATH
  SET RETVAL=%~f1
  EXIT /B