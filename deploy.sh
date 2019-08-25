#! /usr/bin/env sh

# harvest dlls sdkencryptedappticket64.dll ncclient.dll steam_api64.dll

cp ./deps/libncclient/ncclient.dll ./build_root
cp ./bin/Release/crapmud_client.exe ./build_root

cd build_root

sh ../deploy_gather_dlls.sh ./crapmud_client.exe .

cp * ../Release/bin

