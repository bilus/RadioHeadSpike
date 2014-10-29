echo Screen %1
@echo off
REM Serial port config (-sercfg) parameters are: baud rate, data bits, parity, stop bit and flow control respectively.
cd .. && putty.exe -serial %1 -sercfg 9600,8,n,1,N