@echo off
if not exist ..\build mkdir ..\build
pushd ..\build
      rc -fomkeys.res -v ..\code\mkeys.rc
      cl -nologo -FC -Zi ..\code\mkeys.cpp mkeys.res user32.lib shell32.lib
popd
