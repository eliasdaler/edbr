# MTP release

Might do some CI/automatization later, but I'm lazy.

```sh
# start from repo's root
cmake -B build/mtp_release -S games/mtp -DCMAKE_INSTALL_PREFIX=build/mtp_release/install -DBUILD_SHARED_LIBS=OFF
```

## Windows

```sh
./scripts/mtp_release_win32.bat
```

## Linux

```sh
./scripts/mtp_release_linux.sh
```
