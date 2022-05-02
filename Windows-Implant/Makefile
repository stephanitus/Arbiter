all:
	g++ -g -static -static-libgcc -static-libstdc++ httpnode.cpp	-Iinclude	-lwinhttp -liphlpapi			-o bin/httpnode.exe
	g++ -g -static -static-libgcc -static-libstdc++ smbnode.cpp	-Iinclude	-liphlpapi						-o bin/smbnode.exe
	g++ -g -static -static-libgcc -static-libstdc++ loader.cpp 				-lshlwapi -lwininet -lurlmon 	-o bin/loader.exe