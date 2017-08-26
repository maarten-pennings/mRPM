@ECHO off
REM 2017 Aug 26 Maarten Pennings


SETLOCAL EnableExtensions EnableDelayedExpansion
REM 'EnableDelayedExpansion' enables !xx! next to %xx%.
REM The !xx! is executed for each occurrence, %xx% is executed per block.
REM A block is either everything between ( and ), or otherwise a line.


REM === Logging =============================================================


SET LOGFILE=%cd%\flash.log
ECHO.Creating new log file > %LOGFILE%
FOR /F "tokens=* USEBACKQ" %%F IN (`DATE /T`) DO SET log_date=%%F
FOR /F "tokens=* USEBACKQ" %%F IN (`TIME /T`) DO SET log_time=%%F
SET log_dir=%cd%
CALL :log "  Started: %log_date% %log_time%" 0
CALL :log "  Directory: %log_dir%" 0
CALL :log "" 0


REM === Find the flash tool =================================================


SET tool_base=%appdata%\..\Local\Arduino15\packages\esp8266\tools\esptool\
IF EXIST !tool_base! (
  FOR /F "tokens=* USEBACKQ" %%F IN (`dir /b !tool_base!`) DO SET tool_sub=%%F\
  SET tool_full=!tool_base!!tool_sub!esptool.exe
  IF EXIST !tool_full! (
    CALL :log "Flash tool found - existing one"
    CALL :log "  !tool_full!"
    CALL :log ""
    GOTO :tool_done
  ) ELSE (
    CALL :log "WARNING This PC has Arduino and ESP8266 installed; failed to find:"
    CALL :log "  !tool_full!"
  )
)

SET tool_base=%cd%\
SET tool_full=!tool_base!esptool.exe
IF EXIST !tool_full! (
  CALL :log "Flash tool found"
  CALL :log "  !tool_full! "
  CALL :log ""
) ELSE (
  SET fail_msg=Failed to find ESPTOOL.EXE in script directory: !tool_full!
  GOTO :abort
)

:tool_done


REM === Find the firmware ===================================================


SET firm_base=%appdata%\..\Local\Temp\
FOR /F "tokens=* USEBACKQ" %%F IN (`dir /b /o /d !firm_base!arduino_build_* 2^> nul`) DO SET firm_sub=%%F\
SET firm_base=!firm_base!!firm_sub!
FOR /F "tokens=* USEBACKQ" %%F IN (`dir /b /o /d !firm_base!*.bin 2^> nul`) DO SET firm_file=%%F
SET firm_full=!firm_base!!firm_file!
IF EXIST !firm_full! (
  CALL :log "Firmware found - in temporary Arduino build dir"
  CALL :log "  !firm_full!"
  CALL :log ""
  GOTO :firm_done
)

SET firm_base=%cd%\
FOR /F "tokens=* USEBACKQ" %%F IN (`dir /b !firm_base!*.bin`) DO SET firm_file=%%F
SET firm_full=!firm_base!!firm_file!
IF EXIST !firm_full! (
  CALL :log "Firmware found"
  CALL :log "  !firm_full!"
  CALL :log ""
) ELSE (
  SET fail_msg=Failed to find firmware: !firm_base!*.bin
  GOTO :abort
)


:firm_done


REM === Get the COM ports ===================================================


SET com_all=
SET com_last=none
SET com_count=0
FOR /F "tokens=* USEBACKQ" %%F IN (`mode ^| find "COM"`) DO (
  SET com_line=%%F
  IF "Q!com_line:Status for device COM=!" == "Q!com_line!" (
    REM Line does not have form 'Status for device COMxx' - ignore
  ) ELSE (
    SET com_port=!com_line:~18,-1!
    SET com_last=!com_port!
    SET com_all=!com_all! !com_port! &REM with trainling space
    SET /a com_count=!com_count!+1
  )
)

CALL :log "COM ports found:%com_all%"

IF %com_count% EQU 0 (
  SET fail_msg=No COM ports found; please connect ESP8266/NodeMCU via USB to PC
  GOTO :abort
)

IF %com_count% EQU 1 (
  SET com_sel=!com_last!
  CALL :log "  Auto selected: !com_sel!"
  GOTO :selected
) ELSE (
  REM Next blocks should be in else, but the () break the %% in !com_all:%com_sel%=!
)

CALL :input "Enter COM port for ESP8266/NodeMCU\n\nAvailable ports: %com_all%" "Enter COM port" "%com_last%"
SET com_sel=!input!
IF "Q%com_sel%" == "Q" (
  SET fail_msg=Aborted by user
  GOTO :abort
)

CALL :log "  User selected: !com_sel!"
IF "Q!com_all: %com_sel% =!" == "Q!com_all!" (
  SET fail_msg=Selected COM port '%com_sel%' not in list '%com_all%'
  GOTO :abort
)

:selected
CALL :log ""


REM === Execute flashing ===================================================


SET cmd=%tool_full%  -cd nodemcu  -cb 512000  -cp %com_sel%  -cf %firm_full%
CALL :log "Command"
CALL :log "  %cmd%"
CALL :message "Confirm flashing of '%firm_file%' to '%com_sel%'\nNote: flashing will take up to 30 seconds."  1  "Confirm"
SET cmd_ack=%input%
IF %cmd_ack% NEQ 1 (
  SET fail_msg=Aborted by user
  GOTO :abort
)

ECHO.============================================================================================ >> %LOGFILE%
%cmd% >> %LOGFILE%
ECHO.============================================================================================ >> %LOGFILE%
IF %ERRORLEVEL% GTR 0 (
  SET fail_msg=Flashing failed
  GOTO :abort
) 
FOR /f "tokens=* delims=" %%g in ('findstr "100" %LOGFILE%') DO SET cmd_result=%%g
IF "Q!cmd_result:100=!" == "Q!cmd_result!" (
  SET fail_msg=Image was not flashed, or not flashed completely
  GOTO :abort
)


REM === Terminate ===========================================================


:success
CALL :log ""
CALL :log "Completed successfully"
CALL :log "  See %LOGFILE% for details"
CALL :message "Success; see %LOGFILE% for log."  0  "Information"
EXIT /B 0


REM === Subroutines =========================================================


:abort
CALL :log "Error: !fail_msg!"
CALL :log ""
CALL :log "Aborted"
CALL :log "  See %LOGFILE% for log"
CALL :message "%fail_msg%"  0  "Critical Error"
EXIT /B 1


:log
REM Add entry to log file and console; call as   CALL :log "entry" ["0"]
IF NOT "Q%~2" == "Q0" ECHO.%~1
ECHO.%~1 >> %LOGFILE%
EXIT /B


:message
REM Show a message with buttons; call as   CALL :message "message" "buttons" "title"   reply comes in   %input%
ECHO WScript.Echo MsgBox(Replace(WScript.Arguments(0),^"\n^",vbNewline),WScript.Arguments(1),WScript.Arguments(2)) > "%temp%\flash.messagebox.vbs"
REM ECHO WScript.Echo MsgBox(WScript.Arguments(0),WScript.Arguments(1),WScript.Arguments(2)) > "%temp%\flash.messagebox.vbs"
FOR /f "tokens=* delims=" %%g in ('cscript //Nologo "%temp%\flash.messagebox.vbs" "%~1" "%~2" "flash.cmd - %~3"') DO SET input=%%g
DEL "%temp%\flash.messagebox.vbs"
EXIT /B


:input 
REM Prompt for input; call as   CALL :input "prompt" "title" "default"  reply comes in   %input% (empty on cancel)
SET input=
ECHO WScript.Echo "A"^&InputBox(Replace(WScript.Arguments(0),^"\n^",vbNewline),WScript.Arguments(1),WScript.Arguments(2)) > "%temp%\flash.inputbox.vbs"
FOR /f "tokens=* delims=" %%g in ('cscript //Nologo "%temp%\flash.inputbox.vbs" "%~1" "flash.cmd - %~2" "%~3"') DO SET input=%%g
SET input=%input:~1%
DEL "%temp%\flash.inputbox.vbs"
EXIT /B


REM last confirm more text and new lines
