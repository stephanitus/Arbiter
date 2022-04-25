all:
	gcc initial.c libzmq.dll	-Iinclude	-lwinhttp -liphlpapi	-o bin/initial.exe
	gcc loader.c -lshlwapi -lwininet -lurlmon -o bin/loader.exe