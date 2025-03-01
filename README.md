# drone-zone 🐝
Experimenting with game development using the dinosaur of programming languages, i.e. C.

### Compilation
```bash
gcc -o dronezone dronezone.c -lm $(sdl2-config --cflags --libs) $(pkg-config --cflags --libs SDL2_ttf SDL2_gfx)
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
