
all: injector hook

injector: 
	i686-w64-mingw32-g++ -static -O2 -o injector.exe injector.cpp
hook:
	i686-w64-mingw32-g++ -shared -static -O2 -o hook.dll hook.cpp