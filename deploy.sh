#! /usr/bin/env sh

# harvest dlls sdkencryptedappticket64.dll steam_api64.dll

rm -f ./build_root/*.dll
rm -f ./build_root/*.exe

mkdir ./Release
mkdir ./Release/bin
rm -rf ./Release/bin/*
mkdir ./Release/bin/scripts
mkdir ./Release/bin/example_scripts

cp ./deps/steamworks_sdk_148a/sdk/redistributable_bin/win64/steam_api64.dll ./build_root
cp ./bin/Deploy/crapmud_client.exe ./build_root

cd build_root

sh ../deploy_gather_dlls.sh ./crapmud_client.exe .

cp -r * ../Release/bin

cd ..
cd ./deps/steamworks_sdk_148a/sdk/tools/ContentBuilder
./run_build.bat