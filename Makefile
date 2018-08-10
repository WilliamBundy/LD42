disabled=-fno-strict-aliasing \
		 -Wno-incompatible-pointer-types-discards-qualifiers \
		 -Wno-parentheses \
		 -Wno-format
all: bin/ld42 Makefile

bin/wq: src/ptWorkQueue.c
	clang -x c -g src/ptWorkQueue.c -o bin/wq
.SILENT:
usr/lib/wpl.o: src/wpl/* src/wpl/thirdparty/*
	#start
	clang -x c $(disabled) -g -c src/wpl/wpl.c -F"usr/lib" -msse2 -DWPL_SDL_BACKEND -o usr/lib/wpl.o
	#end

bin/ld42: usr/lib/wpl.o src/*
	#start
	clang -x c $(disabled) -g -c src/main.c -F"usr/lib" -msse2 -o usr/lib/ld42.o
	#end
	#start
	clang usr/lib/wpl.o usr/lib/ld42.o \
		-msse2 -F"usr/lib" \
		-framework SDL2 -framework OpenGL \
		-o bin/ld42
	#end


.PHONY: clean run bundle
run: bin/ld42
	bin/ld42 &>/dev/null &

clean:
	rm -f bin/ld42
	rm -f usr/lib/ld42.o
	rm -f usr/lib/wpl.o

bundle: 
	mkdir bin/LD42.app
	mkdir bin/LD42.app/Content/
	mkdir bin/LD42.app/Content/Resources
	mkdir bin/LD42.app/Content/Frameworks
	mkdir bin/LD42.app/Content/MacOS
	cp -R usr/lib/SDL2.framework bin/LD42.app/Content/Frameworks
	cp bin/ld42 bin/LD42.app/Content/MacOS
	cp -R bin/assets bin/LD42.app/Content/MacOS

install:
	echo "Copying SDL2.framework to ~/Library/Frameworks"
	cp -R usr/lib/SDL2.framework ~/Library/Frameworks
