# db-proxy
Simple MySQL proxy with query logging

## Limitations
1. Partial support for prepared statements
2. Linux support is missing
3. Tested only with MySQL 5.7

## Installation
### Windows

#### Requirments
* [cmake 3.9+](https://cmake.org)
* [vcpkg](https://github.com/Microsoft/vcpkg)
* [boost 1.66](https://boost.org)

#### Build steps
```console
> vcpkg install boost-asio:x64-windows
> cmake --CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake --DCMAKE_BUILD_TYPE=Release
> cmake --build . --config Release
```