#!/bin/bash
set -ex
rm -f build/mtp_release/mtp_linux.zip
cmake --build build/mtp_release --target install
pushd build/mtp_release/install
strip -s mtpgame/mtpgame
zip -r ../mtp_linux.zip mtpgame
popd
