all:
	gcc initial.c libzmq.dll sqlite3.dll	-Iinclude	-lwinhttp -liphlpapi -lcrypt32 	-o bin/initial.exe
	gcc loader.c -lshlwapi -lwininet -lurlmon -o bin/loader.exe