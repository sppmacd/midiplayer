# Build instructions for MIDIPlayer

## 1. Install dependencies

The dependencies required are:
- A C++20 compiler (GCC 11+, Clang 13+)
- GNU Make / Ninja
- CMake 3.13+
- SFML 2.5.1+
- fmtlib 8.0.0+

Arch Linux:
```
# pacman -S base-devel cmake sfml fmt
```

## 2. Download the project

```sh
git clone https://github.com/sppmacd/midiplayer.git
```

## 3. Run CMake

```sh
mkdir build
cd build
cmake ..
```

You can also use Ninja, then replace the last command with:
```
cmake .. -GNinja
```

## 4. Actually build

If you just want to build, run
```sh
make -j$(nproc)
```

If you want to make a global installation, run
```sh
make install -j$(nproc)
```

You can use `CMAKE_INSTALL_PREFIX` to change the path. See [CMake docs](https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.html) for more details about installing.

## Local and global installations

Local installations can be run using `build/midiplayer` (using `res` directory from the current directory), global with just `midiplayer` (using global resource directory from `CMAKE_INSTALL_PREFIX` and `./res` as fallback.

You can set an installation to be "portable" using `MIDIPLAYER_PORTABLE_INSTALL`. This will override `CMAKE_INSTALL_PREFIX` to `build/root` and create a ready-to-zip directory there. This supports running `midiplayer` from `bin` (as just `./midiplayer`) or from root (as `bin/midiplayer`).
