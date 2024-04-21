#!/bin/bash
set -ex
rm -f build/mtp_release/mtp_linux.zip
pushd build/mtp_release/install
strip -s mtpgame/mtpgame
zip -r ../mtp_linux.zip mtpgame
popd
