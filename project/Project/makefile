# ----------
# makefile
# ----------

# Output name
EXE=final

# Main target
all: final

# --- Platform configs ---
# Windows
ifeq "$(OS)" "Windows_NT"
CFLG=-O3 -Wall -DUSEGLEW         # Windows compile flags
LIBS=-lfreeglut -lglew32 -lglu32 -lopengl32 -lm  # Windows libraries
CLEAN=rm -f *.exe *.o *.a        # Windows cleanup
else
# macOS
ifeq "$(shell uname)" "Darwin"
RES=$(shell uname -r|sed -E 's/(.).*/\1/'|tr 12 21)  # Retina display scaling
CFLG=-O3 -Wall -Wno-deprecated-declarations -DRES=$(RES)  # macOS flags
LIBS=-framework GLUT -framework OpenGL     # macOS frameworks
# Linux/Unix
else
<<<<<<< Updated upstream
CFLG=-O3 -Wall                   # Standard flags
LIBS=-lglut -lGLU -lGL -lm       # Unix libraries
=======
CFLG=-O3 -Wall -DSDL2
LIBS=-lSDL2 -lSDL2_mixer -lglut -lGLU -lGL -lm
>>>>>>> Stashed changes
endif
# Non-Windows cleanup
CLEAN=rm -f $(EXE) *.o *.a
endif

# --- Dependencies ---
# Core components
main.o: main.c CSCIx229.h landscape.h objects.h weather_particles.h
landscape.o: landscape.c landscape.h CSCIx229.h
objects.o: objects.c objects.h landscape.h CSCIx229.h
weather_particles.o: weather_particles.c weather_particles.h landscape.h CSCIx229.h

# Utility library
fatal.o: fatal.c CSCIx229.h
errcheck.o: errcheck.c CSCIx229.h
print.o: print.c CSCIx229.h
loadtexbmp.o: loadtexbmp.c CSCIx229.h
projection.o: projection.c CSCIx229.h

<<<<<<< Updated upstream
# Additional modules
shaders.o: shaders.c CSCIx229.h
sky.o: sky.c sky.h landscape.h
sky_clouds.o: sky_clouds.c sky_clouds.h
camera.o: camera.c camera.h

# Create utility library
CSCIx229.a: fatal.o errcheck.o print.o loadtexbmp.o loadobj.o projection.o
	ar -rcs $@ $^

=======
>>>>>>> Stashed changes
# Compile rules
.c.o:
	gcc -c $(CFLG) $<
.cpp.o:
	g++ -c $(CFLG) $<

<<<<<<< Updated upstream
# Link executable
final: main.o landscape.o objects.o weather_particles.o shaders.o sky.o sky_clouds.o camera.o sky_clouds.o CSCIx229.a
	gcc $(CFLG) -o $@ $^ $(LIBS)
=======
#  Link
final: main.o landscape.o shaders.o sky.o sky_clouds.o camera.o fractal_tree.o objects_render.o particles.o boulder.o grass.o sound.o fatal.o print.o loadtexbmp.o projection.o errcheck.o
	$(CC) $(CFLG) -o $@ $^ $(LIBS)
>>>>>>> Stashed changes

# Cleanup
clean:
	$(CLEAN)