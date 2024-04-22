# MTP release

Might do some CI/automatization later, but I'm lazy.

Dependencies:

* 7-zip (Windows only)
* Butler (itch.io)

1. Update version in dev/itch_version.txt
2. Update version in Game::loadAppSettings

Configure CMake (only needs to be done once)
```sh
# start from repo's root
cmake -B build/mtp_release -S games/mtp -DCMAKE_INSTALL_PREFIX=build/mtp_release/install -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release
```

## Windows

```sh
./scripts/mtp_release_win32.bat
```

## Linux

```sh
./scripts/mtp_release_linux.sh
```
