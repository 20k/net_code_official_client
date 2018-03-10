copy "bin\\Deploy\\crapmud_client.exe" "bin\\Deploy\\crapmud_client_strip.exe"
copy "bin\\Deploy\\crapmud_client.exe" "Release\\bin"
copy "libduktape.so.202.20200" "Release\\bin"
copy "VeraMono.ttf" "Release\\bin"
cv2pdb.exe bin/Deploy/crapmud_client_strip.exe
rename "bin\\Deploy\\crapmud_client_strip.pdb" crapmud_client.pdb
copy "bin\\Deploy\\crapmud_client.pdb" "Release\\bin"
del "bin\\Deploy\\crapmud_client.pdb"