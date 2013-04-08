
set INSTALL_DIR=..\HPServer
set INSTALL_DIR_INC=..\HPServer\include
set INSTALL_DIR_LIB=..\HPServer\lib
set INSTALL_DIR_BIN=..\HPServer\bin

@ echo Install dir = %INSTALL_DIR%
REM copy include files
mkdir %INSTALL_DIR% 
mkdir %INSTALL_DIR_INC% 
mkdir %INSTALL_DIR_LIB% 
mkdir %INSTALL_DIR_BIN%
@ del %INSTALL_DIR_INC%\win32 /F /Q
mkdir %INSTALL_DIR_INC%\win32
xcopy ..\include\HP_Config.h         %INSTALL_DIR_INC%\win32\ /Y
xcopy ..\include\defines.h           %INSTALL_DIR_INC%\win32\ /Y
xcopy ..\include\win32\SockUtil.h    %INSTALL_DIR_INC%\win32\ /Y
xcopy ..\include\TimeUtil.h    	     %INSTALL_DIR_INC%\win32\ /Y
xcopy ..\include\inetAddr.h          %INSTALL_DIR_INC%\win32\ /Y
xcopy ..\include\Log.h               %INSTALL_DIR_INC%\win32\ /Y
xcopy ..\include\SockAcceptor.h      %INSTALL_DIR_INC%\win32\ /Y
xcopy ..\include\Connector.h         %INSTALL_DIR_INC%\win32\ /Y
xcopy ..\include\EventHandler.h      %INSTALL_DIR_INC%\win32\ /Y
xcopy ..\include\Reactor.h           %INSTALL_DIR_INC%\win32\ /Y
xcopy ..\include\Reactor_Imp.h       %INSTALL_DIR_INC%\win32\ /Y
xcopy ..\include\EventScheduler.h    %INSTALL_DIR_INC%\win32\ /Y
REM copy libraries
mkdir %INSTALL_DIR_LIB%\win32	
xcopy ..\bin\*.dll   %INSTALL_DIR_LIB%\win32\ /Y
mkdir %INSTALL_DIR_BIN%\win32	
xcopy ..\bin\*.lib   %INSTALL_DIR_BIN%\win32\ /Y

pause