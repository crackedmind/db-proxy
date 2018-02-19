# db-proxy
Simple MySQL proxy with query logging

## Limitations
1. Partial support for prepared statements
2. Linux support is missing
3. Tested only with MySQL 5.7

## Installation
### Windows

#### Requirments
* [Visual C++ 15.5](https://docs.microsoft.com/en-us/visualstudio/install/install-visual-studio) (not tested on previous versions)
* [cmake 3.11](https://cmake.org) - CMake versions prior to 3.11 doesn't support Boost 1.66, or update FindBoost.cmake module manually from master branch
* [vcpkg](https://github.com/Microsoft/vcpkg)
* [boost 1.66](https://boost.org)

#### Build steps
```console
> vcpkg install boost-asio:x64-windows
> mkdir build && cd build
> cmake --CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake --DCMAKE_BUILD_TYPE=Release ..
> cmake --build . --config Release
```
