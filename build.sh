if find . -name "build"; then
rm  -rf build
fi
mkdir build
cmake -S . -B build
cmake --build build
