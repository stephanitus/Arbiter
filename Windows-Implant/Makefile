all:
	g++ initial.cpp aesgcm.cpp sqlite3.dll	-Iinclude	-lwinhttp -liphlpapi -lcrypt32 -lbcrypt	-o bin/initial.exe
	gcc loader.c -lshlwapi -lwininet -lurlmon -o bin/loader.exe