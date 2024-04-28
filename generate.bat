@echo off
conan install . -if build -s build_type=Release -s compiler.version=16 --build missing
utils\bin\premake5 %* vs2022
