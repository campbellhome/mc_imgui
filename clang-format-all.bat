wsl clang-format -i include/*.h src/*.c*
pushd submodules\mc_common
call clang-format-all.bat
popd