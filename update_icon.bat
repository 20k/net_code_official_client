ffmpeg -i test_icon.png icon.ico
rm icon.res
windres icon.rc -O coff -o icon.res
cp icon.res ./obj/deploy