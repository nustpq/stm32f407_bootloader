cls
REM ------------------------------------------------------------------------------------
REM Step 0: get path and make tmp folder
REM ------------------------------------------------------------------------------------
pyinstaller --clean -F -i window_fullscreen.ico --version-file=file_version_info.txt flasher.py
REM ------------------------------------------------------------------------------------
echo Generating package finished!
pause./