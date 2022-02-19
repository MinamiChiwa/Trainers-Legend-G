@echo off
conan install . -if build -s build_type=Release
utils\bin\premake5 %* vs2022
