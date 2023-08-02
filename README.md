# Planet

Just testing vulkan on my macbook.

Build should work cross platform though as well by just changing a few flags.
Will test on windows at some point.

## Build with cmake

```sh
cmake . -B build
```

## Build and run (currently testing on moltenvk)

```sh
cmake --build build && ./build/Planet
```

# Generate the compile_commands.json for LSP

```sh
cmake . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1
```

## Build on mac

```sh
brew install fmt spdlog
```
