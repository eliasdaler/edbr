rm ./build/mtp_release/mtp_win32.zip
cmake --build build/mtp_release --target install --config Release
"C:/Program Files/7-Zip/7z.exe" a build/mtp_release/mtp_win32.zip ./build/mtp_release/install/mtpgame