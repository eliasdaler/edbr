# MTP release

Might do some CI/automatization later, but I'm lazy.

```sh
# start from repo's root
cmake -B build/mtp_release -S games/mtp -DCMAKE_INSTALL_PREFIX=build/mtp_release/install -DBUILD_SHARED_LIBS=OFF
```

## Windows

```cmake --build build/mtp_release --target install --config Release```

Zip build/mtp_release/install/mtpgame (in Git Bash, 7-zip should be installed):

```sh
/c/Program\ Files/7-Zip/7z.exe a build/mtp_release/mtp_win32.zip ./build/mtp_release/install/mtpgame
```

## Linux

```sh
cmake --build build/mtp_release --target install
./scripts/mtp_linux_release.sh
```
