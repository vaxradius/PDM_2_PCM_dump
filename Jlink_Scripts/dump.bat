@echo off

if [%1]==[] goto usage

jlink -device AMA3B1KK-KBR -CommanderScript dump.jlink
rename dump.bin %1
pause
goto :eof
:usage
@echo Usage: %0 ^<output file name^>
exit /B 1
