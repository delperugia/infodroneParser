
# Compilation

cmake      -B         build/
cmake      --build    build/ -j

# Test

./build/infodroneParser tests/anafi-infodrone.pcapng
