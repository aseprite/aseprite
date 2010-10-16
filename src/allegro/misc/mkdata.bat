@echo off
rem Create `languages.dat' and `keyboard.dat' from `resource' directory.

echo Creating keyboard.dat...
cd resource\keyboard
dat -a -c1 ..\..\keyboard.dat *.cfg
cd ..\..

echo Creating languages.dat...
cd resource\language
dat -a -c1 ..\..\language.dat *.cfg
cd ..\..
