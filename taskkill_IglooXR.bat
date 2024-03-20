@echo off
for /f "tokens=1" %%i in ('tasklist ^| findstr /i "_IglooXR"') do (
    taskkill /f /im "%%i"
)
