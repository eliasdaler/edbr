rm ./build/mtp_release/mtp_win32.zip
cmake --build build/mtp_release --target install --config Release
"C:/Program Files/7-Zip/7z.exe" a build/mtp_release/mtp_win32.zip ./build/mtp_release/install/mtpgame
butler push build/mtp_release/mtp_win32.zip eliasdaler/project-mtp-dev:win32 --userversion-file games/mtp/dev/itch_version.txt
