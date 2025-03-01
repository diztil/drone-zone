# drone-zone
Experimenting with game development using the dinosaur of programming languages, i.e. C.

### Compilation
```bash
gcc -o dronezone dronezone.c -lm $(sdl2-config --cflags --libs) $(pkg-config --cflags --libs SDL2_ttf)
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
- [ ] Keyboard controls
- [ ] GUI health bar
- [ ] GUI score board
- [ ] File I/O for saving/retrieving high scores
- [ ] Boids algorithm for drone simulation    
