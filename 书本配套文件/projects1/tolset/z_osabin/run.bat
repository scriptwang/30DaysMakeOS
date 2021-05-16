@echo off
..\tolset\z_tools\make.exe -r -C ../tolset/z_tools/osa_qemu
if %1.==. goto def_opt
if %1.==.. goto no_opt
copy %1.bin ..\tolset\z_tools\!built.bin
..\tolset\z_tools\edimg.exe @../tolset/z_tools/edimgopt.txt
if errorlevel 1 goto end
goto qemu
:def_opt
..\tolset\z_tools\edimg.exe @!run_opt.txt
if errorlevel 1 goto end
goto qemu
:no_opt
copy ..\tolset\z_tools\osa_qemu\osaimgqe.bin ..\tolset\z_tools\qemu\fdimage0.bin
:qemu
..\tolset\z_tools\make.exe -r -C ../tolset/z_tools/qemu
:end
