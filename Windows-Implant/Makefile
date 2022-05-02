all:
	g++ -g -static-libstdc++ httpnode.cpp	-Iinclude	-lwinhttp -liphlpapi			-o bin/httpnode.exe
	g++ -g -static-libstdc++ smbnode.cpp	-Iinclude	-liphlpapi						-o bin/smbnode.exe
	g++ -g -static-libstdc++ loader.cpp 				-lshlwapi -lwininet -lurlmon 	-o bin/loader.exe