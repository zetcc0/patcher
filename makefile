all: injector.exe hook.dll

injector.exe: injector.cpp
	i686-w64-mingw32-g++ -static -O2 -o $@ $^

hook.dll: main.cpp hook.cpp hook.h
	i686-w64-mingw32-g++ -shared -static -O2 -o $@ main.cpp hook.cpp

clean:
	del /Q injector.exe hook.dll 2>NUL