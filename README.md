# Planet

## Build with cmake

```sh
cmake . -B build
make -C build
```

# Generate the compile_commands.json for LSP

```sh
cmake . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1
```

## Build on mac
```sh
brew install fmt spdlog
```

# Compile shaders

```sh
glslangValidator -V shaders/planet.frag -o shaders/planet.spv
glslangValidator -V shaders/fullscreenquad.vert -o shaders/fullscreenquad.spv
```

- [ ]  TODO: Update to include linking moltenvk directly because it's needed for dynamic rendering

### g++
```sh
# Debug
g++ -std=c++20 -I/opt/homebrew/Cellar/fmt/10.0.0/include -I/opt/homebrew/Cellar/spdlog/1.11.0_1/include/ -L/opt/homebrew/Cellar/fmt/10.0.0/lib/ -L/opt/homebrew/Cellar/fmt/10.0.0/lib -L/opt/homebrew/Cellar/spdlog/1.11.0_1/lib -lfmt -lspdlog -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wunused -pedantic main.cpp -g -o build/planet

# Release
g++ -std=c++20 -I/opt/homebrew/Cellar/fmt/10.0.0/include -I/opt/homebrew/Cellar/spdlog/1.11.0_1/include/ -L/opt/homebrew/Cellar/fmt/10.0.0/lib/ -L/opt/homebrew/Cellar/fmt/10.0.0/lib -L/opt/homebrew/Cellar/spdlog/1.11.0_1/lib -lfmt -lspdlog -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wunused -pedantic main.cpp -O3 -o build/planet
```

### clang++
```sh
# Debug
clang++ -std=c++20 -I/opt/homebrew/Cellar/fmt/10.0.0/include -I/opt/homebrew/Cellar/spdlog/1.11.0_1/include/ -L/opt/homebrew/Cellar/fmt/10.0.0/lib/ -L/opt/homebrew/Cellar/spdlog/1.11.0_1/lib -lfmt -lspdlog -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wunused -pedantic main.cpp -g -o build/planet

# Release
clang++ -std=c++20 -I/opt/homebrew/Cellar/fmt/10.0.0/include -I/opt/homebrew/Cellar/spdlog/1.11.0_1/include/ -L/opt/homebrew/Cellar/fmt/10.0.0/lib/ -L/opt/homebrew/Cellar/spdlog/1.11.0_1/lib -lfmt -lspdlog -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wunused -pedantic main.cpp -O3 -o build/planet
```
