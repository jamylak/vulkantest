# Planet

## Build

```sh
cmake . -B build
make -C build
```

# Generate the compile_commands.json for LSP

```sh
cmake . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1
```
