#! /usr/bin/env sh

# harvest dlls sdkencryptedappticket64.dll ncclient.dll steam_api64.dll

rm ./build_root/*.dll
rm ./build_root/*.exe

mkdir ./Release
mkdir ./Release/bin
rm -r ./Release/bin/*

cp ./deps/steamworks_sdk_142/sdk/redistributable_bin/win64/steam_api64.dll ./build_root
cp ./deps/libncclient/ncclient.dll ./build_root
cp ./bin/Deploy/crapmud_client.exe ./build_root

cd build_root

sh ../deploy_gather_dlls.sh ./crapmud_client.exe .

cp * ../Release/bin

