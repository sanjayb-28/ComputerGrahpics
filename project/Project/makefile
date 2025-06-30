EXE=final

# Main target
all: $(EXE)

#  Msys/MinGW
ifeq "$(OS)" "Windows_NT"
CFLG=-O3 -Wall -DSDL2
LIBS=-lmingw32 -lSDL2main -lSDL2 -mwindows -lSDL2_mixer -lglut -lglu32 -lopengl32 -lm
CLEAN=rm -f *.exe *.o *.a
else
#  OSX
ifeq "$(shell uname)" "Darwin"
CFLG=-O3 -Wall -Wno-deprecated-declarations -DSDL2 -I/opt/homebrew/include/SDL2 -I/opt/homebrew/include
LIBS=-L/opt/homebrew/lib -lSDL2 -lSDL2_mixer -lGLEW -framework GLUT -framework Cocoa -framework OpenGL
#  Linux/Unix/Solaris
else
# --- GLEW ENABLED (uncomment these two lines to enable GLEW) ---
#CFLG=-O3 -Wall -DSDL2 -DUSEGLEW
#LIBS=-lSDL2 -lSDL2_mixer -lglut -lGLEW -lGLU -lGL -lm
# --- GLEW DISABLED (default, leave these lines uncommented for no GLEW) ---
CFLG=-O3 -Wall -DSDL2
LIBS=-lSDL2 -lSDL2_mixer -lglut -lGLU -lGL -lm
endif
#  OSX/Linux/Unix/Solaris
CLEAN=rm -f $(EXE) *.o *.a
endif

# Dependencies
main.o: main.c CSCIx229.h landscape.h
landscape.o: landscape.c landscape.h CSCIx229.h
shaders.o: shaders.c CSCIx229.h
sky.o: sky.c sky.h landscape.h
sky_clouds.o: sky_clouds.c sky_clouds.h
camera.o: camera.c camera.h
fractal_tree.o: fractal_tree.c fractal_tree.h
objects_render.o: objects_render.c objects_render.h landscape.h fractal_tree.h
particles.o: particles.c particles.h
boulder.o: boulder.c boulder.h
grass.o: grass.c grass.h
sound.o: sound.c sound.h
fatal.o: fatal.c CSCIx229.h
errcheck.o: errcheck.c CSCIx229.h
print.o: print.c CSCIx229.h
loadtexbmp.o: loadtexbmp.c CSCIx229.h
loadobj.o: loadobj.c CSCIx229.h
projection.o: projection.c CSCIx229.h

#  Create archive
CSCIx229.a:fatal.o errcheck.o print.o loadtexbmp.o loadobj.o projection.o
	ar -rcs $@ $^

# Compile rules
.c.o:
	gcc -c $(CFLG)  $<
.cpp.o:
	g++ -c $(CFLG)  $<

#  Link
final: main.o landscape.o shaders.o sky.o sky_clouds.o camera.o fractal_tree.o objects_render.o particles.o boulder.o grass.o sound.o CSCIx229.a
	$(CC) $(CFLG) -o $@ $^ $(LIBS)

#  Clean
clean:
	$(CLEAN)