# drone-zone 🐝
Experimenting with game development using the dinosaur of programming languages, i.e. C.

### Dependencies
```bash
sudo apt install build-essential libsdl2-dev libsdl2-ttf-dev libsdl2-gfx-dev libm-dev
```
As of now, the program needs `Sigmar-Regular.ttf` to be present in the same directory (otherwise weird errors would occur). Be sure to download it from the repository code & files above.

### Compilation
##### Linux 🐧
```bash
gcc -o dronezone dronezone.c -lm $(sdl2-config --cflags --libs) $(pkg-config --cflags --libs SDL2_ttf SDL2_gfx)
```
as simple as that!
##### Windows 🪟
```powershell
gcc -o dronezone dronezone.c $(sdl2-config --cflags) -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_gfx -lm
```
via `MSYS2`, after installing the necessary libraries, i.e.:
```bash
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL2_gfx
```
and then make sure to add these to `PATH` via System Environment Variables:
```
C:\mingw-w64\bin
C:\msys64\mingw64\bin
```

### Launching
```bash
 ./dronezone 
```

### Debugging
```bash
 gdb ./dronezone 
```
- `Enable debuginfod for this session? (y or [n])` **n**
- `(gdb)` **run**

## Features
- [x] GUI implementation
- [x] GUI buttons with hover effects
- [x] GUI fonts
- [x] Mouse controls
- [x] GUI health bar
- [x] GUI score board
- [x] Multiple GUI screens
- [x] File I/O for saving/retrieving high scores
- [x] Boids algorithm for drone simulation
- [x] Hardcoded bee models
- [x] Hardcoded plants
- [x] Hardcoded dynamic background
