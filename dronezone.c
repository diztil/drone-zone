#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#define WIDTH 800
#define HEIGHT 600
#define MAX_SPEED 4
#define MAX_PLANTS 100
#define NUM_DRONES 50
#define PLAYER_ACCEL 0.5f
#define FRICTION 0.98f
#define FPS 60
#define FRAME_DELAY (1000 / FPS)
#define SEPARATION_RADIUS 25.0f
#define ALIGNMENT_RADIUS 100.0f
#define COHESION_RADIUS 100.0f
#define COHESION_WEIGHT 0.01f
#define ALIGNMENT_WEIGHT 0.05f
#define SEPARATION_WEIGHT 0.1f
#define COHESION_FACTOR 0.01
#define ALIGNMENT_FACTOR 0.05
#define SEPARATION_FACTOR 0.1
#define SEPARATION_DISTANCE 25
#define MAX_CIRCLES 10


typedef struct {
    float x, y, vx, vy;
} Drone;

typedef struct {
    SDL_Rect rect;
    SDL_Color defaultColor;
    SDL_Color hoverColor;
    SDL_Color clickColor;
    int hovered;
    int clicked;
} Button;

typedef struct {
    float x, y;
    int radius;
    SDL_Color color;
    int isVisible;
    int alpha;
    Uint32 lastAppearanceTime; // For fade-in and fade-out effect
} Circle;

typedef struct {
    float x, y;          // Position of the plant base
    float growth;        // Growth stage (0.0 to 1.0)
    float maxHeight;     // Maximum height it can reach
    Uint32 spawnTime;    // When it was created
    Uint32 lifespan;     // How long before it disappears
    SDL_Color color;     // Green initially, turns brown later
    int type;            // 0 = Grass, 1 = Vine, 2 = Fern
    int alpha;
    int isVisible;
    int height;
} Plant;

typedef struct {
    int x, y;
    SDL_Color color;
    int isBloomed;
} Flower;


Uint32 lastCircleSpawnTime = 0;

Drone player;
Drone drones[NUM_DRONES];
Circle circles[MAX_CIRCLES];
Plant plants[MAX_PLANTS];
Flower flowers[MAX_CIRCLES];

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;

int running = 1;
int inGame = 0;
int inHelp = 0;
int gameOver = 0;
int playerHealth = 100;
int score = 0;
int highScore = 0;
int numCircles = 0;
int numPlants = 0;
int grayProgress = 0;

// Define honey-colored palette
SDL_Color honeyPrimary = {255, 186, 77, 255};  // Warm golden
SDL_Color honeySecondary = {230, 153, 51, 255}; // Darker honey
SDL_Color honeyHighlight = {255, 204, 102, 255}; // Lighter honey
SDL_Color honeyShadow = {200, 140, 0, 255};     // Darker brown
SDL_Color beeStripe = {50, 30, 0, 255};        // Dark brown for stripes
SDL_Color beeFace = {0, 0, 0, 255};            // Black for eyes/mouth


// Menu Buttons
Button playButton = {{WIDTH / 2 - 50, 200, 100, 40}, {255, 255, 255, 255}, {200, 200, 200, 255}, {100, 100, 100, 255}, 0, 0};
Button helpButton = {{WIDTH / 2 - 50, 250, 100, 40}, {255, 255, 255, 255}, {200, 200, 200, 255}, {100, 100, 100, 255}, 0, 0};
Button backButton = {{10, HEIGHT - 50, 100, 40}, {255, 255, 255, 255}, {200, 200, 200, 255}, {100, 100, 100, 255}, 0, 0};
Button menuButton = {{WIDTH - 110, HEIGHT - 50, 100, 40}, {255, 255, 255, 255}, {200, 200, 200, 255}, {100, 100, 100, 255}, 0, 0};
Button exitButton = {{WIDTH / 2 - 50, 300, 100, 40}, {255, 255, 255, 255}, {200, 200, 200, 255}, {100, 100, 100, 255}, 0, 0};

// Load high score from file
void loadHighScore() {
    FILE *file = fopen("highscore.txt", "r");
    if (file) {
        fscanf(file, "%d", &highScore);
        fclose(file);
    }
}

// Save high score to file
void saveHighScore() {
    FILE *file = fopen("highscore.txt", "w");
    if (file) {
        fprintf(file, "%d", highScore);
        fclose(file);
    }
}

void renderBackground() {
    // Draw sky
    SDL_SetRenderDrawColor(renderer, 173, 216, 230, 255); // Pale blue
    //SDL_RenderClear(renderer);
    
    // Draw sun
    int sunX = WIDTH - 100, sunY = 80, sunRadius = 40;
    filledCircleRGBA(renderer, sunX, sunY, sunRadius, 255, 223, 0, 55);
    
    // Draw sun rays (trapezoidal shape)
    for (int angle = 0; angle < 360; angle += 30) {
        int x1 = sunX + cos(angle * M_PI / 180) * (sunRadius + 10);
        int y1 = sunY + sin(angle * M_PI / 180) * (sunRadius + 10);
        int x2 = sunX + cos(angle * M_PI / 180) * (sunRadius + 30);
        int y2 = sunY + sin(angle * M_PI / 180) * (sunRadius + 30);
        thickLineRGBA(renderer, x1, y1, x2, y2, 6, 255, 200, 0, 55);
    }

    // Draw layered meadows
    for (int i = 0; i < 1; i++) {
        int meadowY = HEIGHT - (i * 20) - 30;
        filledEllipseRGBA(renderer, WIDTH / 2 - 50, meadowY + 50, WIDTH / 2 + 80, 50, 34 + i * 10, 139 + i * 10, 34, 255);
    }

    // Random tiny flowers in meadows
    int fx = rand() % WIDTH;
    int fy = HEIGHT - (rand() % 120 + 30);
    SDL_Color flowerColor = {rand() % 256, rand() % 256, rand() % 256, 55};
    filledCircleRGBA(renderer, fx, fy, 2, flowerColor.r, flowerColor.g, flowerColor.b, 55);
    
}

void spawnPlants() {
    if (numPlants >= MAX_PLANTS) return; // Prevent overflow

    int x = rand() % WIDTH;
    int type = rand() % 3;  // Random type: Grass, Vine, or Fern

    plants[numPlants].x = x;
    plants[numPlants].y = HEIGHT;  // Always start at the bottom
    plants[numPlants].growth = 0.0f;
    plants[numPlants].maxHeight = (rand() % 40) + 30;  // 30 to 70 pixels
    plants[numPlants].spawnTime = SDL_GetTicks();
    plants[numPlants].lifespan = (rand() % 15000) + 10000; // 10 to 25 sec
    plants[numPlants].color = (SDL_Color){34, 139, 34, 255}; // Green
    plants[numPlants].type = type;

    numPlants++;
}


void updatePlants() {
    for (int i = 0; i < MAX_PLANTS; i++) {
        if (!plants[i].isVisible) continue;
        
        Uint32 elapsed = SDL_GetTicks() - plants[i].spawnTime;
        if (elapsed > 10000) { // Slowly disappear after 10s
            plants[i].color.r = 139;
            plants[i].color.g = 69;
            plants[i].color.b = 19;
            plants[i].alpha -= 5;
            if (plants[i].alpha <= 0) {
                plants[i].isVisible = 0;
            }
        }
    }
}

void renderPlants() {
    Uint32 currentTime = SDL_GetTicks();

    for (int i = 0; i < numPlants; i++) {
        float progress = (currentTime - plants[i].spawnTime) / (float)plants[i].lifespan;

        // Change color to brown if it's near the end of lifespan
        if (progress > 0.8f) {
            plants[i].color.r = 139;
            plants[i].color.g = 69;
            plants[i].color.b = 19;
        }

        // Calculate growth height
        float height = plants[i].growth * plants[i].maxHeight;
        SDL_SetRenderDrawColor(renderer, plants[i].color.r, plants[i].color.g, plants[i].color.b, plants[i].color.a);

        if (plants[i].type == 0) {
            // Grass (short vertical lines)
            SDL_RenderDrawLine(renderer, plants[i].x, HEIGHT, plants[i].x, HEIGHT - height);
        } else if (plants[i].type == 1) {
            // Vine (slightly curving line)
            for (int y = 0; y < height; y += 4) {
                SDL_RenderDrawPoint(renderer, plants[i].x + (rand() % 3 - 1), HEIGHT - y);
            }
        } else {
            // Fern (small diagonal lines)
            for (int y = 0; y < height; y += 5) {
                SDL_RenderDrawLine(renderer, plants[i].x, HEIGHT - y, plants[i].x + (rand() % 8 - 4), HEIGHT - y - 3);
            }
        }

        // Update growth
        if (plants[i].growth < 1.0f) {
            plants[i].growth += 0.01f;
        }

        // Remove expired plants
        if (progress >= 1.0f) {
            plants[i] = plants[numPlants - 1]; // Replace with last plant
            numPlants--;
            i--;
        }
    }
}

void spawnFlower(int x, int y) {
    if (numCircles < MAX_CIRCLES) {
        flowers[numCircles].x = x;
        flowers[numCircles].y = y;
        flowers[numCircles].color = (SDL_Color){rand() % 256, rand() % 256, rand() % 256, 255};
        flowers[numCircles].isBloomed = 0;
        numCircles++;
    }
}

void updateFlowers() {
    for (int i = 0; i < numCircles; i++) {
        if (!flowers[i].isBloomed) {
            // Simulate blooming logic
            flowers[i].isBloomed = 1;
        }
    }
}

void renderFlowers(SDL_Renderer* renderer) {
    for (int i = 0; i < numCircles; i++) {
        filledCircleRGBA(renderer, flowers[i].x, flowers[i].y, 5, 
                         flowers[i].color.r, flowers[i].color.g, flowers[i].color.b, 255);
    }
}

void spawnCircles() {
    Uint32 currentTime = SDL_GetTicks();

    if (currentTime - lastCircleSpawnTime > 10000) { // Every 10 seconds
        lastCircleSpawnTime = currentTime;

        int numNewCircles = rand() % 3 + 1; // 1 to 3 circles

        if (numCircles + numNewCircles > MAX_CIRCLES) {
            numNewCircles = MAX_CIRCLES - numCircles;
        }

        for (int i = 0; i < numNewCircles; i++) {
            int x = rand() % (WIDTH - 20) + 10;
            int y = rand() % (HEIGHT - 20) + 10;
            
            // Grow stem animation
            for (int h = HEIGHT; h > y; h -= 5) {
                SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255);
                SDL_RenderDrawLine(renderer, x, h, x, h - 5);
                SDL_RenderPresent(renderer);
                SDL_Delay(10);
            }
            
            // Grow green bud
            int budRadius = 2;
            while (budRadius < 10) {
                filledCircleRGBA(renderer, x, y, budRadius, 34, 139, 34, 255);
                SDL_RenderPresent(renderer);
                SDL_Delay(30);
                budRadius++;
            }
            
            // Blooming animation
            SDL_Color petalColor = {rand() % 256, rand() % 256, rand() % 256, 255};
            for (int p = 0; p < 360; p += 45) {
                int petalX = x + cos(p * M_PI / 180) * 12;
                int petalY = y + sin(p * M_PI / 180) * 12;
                filledEllipseRGBA(renderer, petalX, petalY, rand() % 8 + 5, rand() % 6 + 4, petalColor.r, petalColor.g, petalColor.b, 255);
                SDL_RenderPresent(renderer);
                SDL_Delay(50);
            }
            
            // Assign final bloom properties
            circles[numCircles].x = x;
            circles[numCircles].y = y;
            circles[numCircles].radius = 10;
            circles[numCircles].color = (SDL_Color){rand() % 100 + 100, rand() % 80 + 60, rand() % 60 + 40, 255}; // Earthy tones
            circles[numCircles].isVisible = 1;
            circles[numCircles].alpha = 255;
            circles[numCircles].lastAppearanceTime = currentTime;
            numCircles++;
        }
    }
}


// Draw a filled circle
void SDL_RenderFillCircle(SDL_Renderer *renderer, int x, int y, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w; 
            int dy = radius - h;
            if (dx*dx + dy*dy <= radius * radius) {
                SDL_RenderDrawPoint(renderer, x + dx, y + dy);
            }
        }
    }
}

// Function to draw a bee (player or drone)
void drawBee(SDL_Renderer *renderer, float x, float y, float vx, float vy, int isPlayer) {
    float angle = atan2(vy, vx);
    SDL_SetRenderDrawColor(renderer, 255, 223, 0, 255); // Yellow bee body
    filledCircleRGBA(renderer, (int)x, (int)y, 10, 255, 223, 0, 255);
    
    // Draw black stripes
    for (int i = -8; i <= 8; i += 4) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawLine(renderer, x - 7, y + i, x + 7, y + i);
    }

    // Draw larger wings
    filledEllipseRGBA(renderer, (int)(x - 10), (int)(y - 5), 10, 5, 200, 200, 255, 180);
    filledEllipseRGBA(renderer, (int)(x + 10), (int)(y - 5), 10, 5, 200, 200, 255, 180);
    filledEllipseRGBA(renderer, (int)(x - cos(angle) * 8), (int)(y - sin(angle) * 5), 6, 3, honeyHighlight.r, honeyHighlight.g, honeyHighlight.b, 180);
    filledEllipseRGBA(renderer, (int)(x + cos(angle) * 8), (int)(y - sin(angle) * 5), 6, 3, honeyHighlight.r, honeyHighlight.g, honeyHighlight.b, 180);
    
    // Draw antennae
    SDL_RenderDrawLine(renderer, (int)x - 3, (int)y - 10, (int)x - 5, (int)y - 15);
    SDL_RenderDrawLine(renderer, (int)x + 3, (int)y - 10, (int)x + 5, (int)y - 15);
    
    // Draw face only for the player
    if (isPlayer) {
        filledCircleRGBA(renderer, (int)x, (int)y, 10, honeyPrimary.r, honeyPrimary.g, honeyPrimary.b, 255);
        filledCircleRGBA(renderer, (int)x - 3, (int)y - 2, 1, 0, 0, 0, 255); // Left eye
        filledCircleRGBA(renderer, (int)x + 3, (int)y - 2, 1, 0, 0, 0, 255); // Right eye
        SDL_RenderDrawLine(renderer, (int)x - 1, (int)y + 1, (int)x + 2, (int)y + 3); // Smile
    }
}

// Initialize drones and circles
void initDrones() {
    srand(time(NULL));
    player.x = WIDTH / 2;
    player.y = HEIGHT / 2;
    player.vx = player.vy = 0;

    // Initialize player attributes
    playerHealth = 100;
    score = 0;

    for (int i = 0; i < NUM_DRONES; i++) {
        drones[i].x = rand() % WIDTH;
        drones[i].y = rand() % HEIGHT;
        drones[i].vx = (float)(rand() % MAX_SPEED) - MAX_SPEED / 2;
        drones[i].vy = (float)(rand() % MAX_SPEED) - MAX_SPEED / 2;
    }

    // Initialize circles
    numCircles = 0;
    for (int i = 0; i < MAX_CIRCLES; i++) {
        circles[i].isVisible = 0;  // Initially not visible
    }
}

// Calculate the separation force (avoid crowding neighbors)
SDL_Point separation(Drone *drone) {
    SDL_Point force = {0, 0};
    int count = 0;

    for (int i = 0; i < NUM_DRONES; i++) {
        if (&drones[i] != drone) {
            float dx = drone->x - drones[i].x;
            float dy = drone->y - drones[i].y;
            float distance = sqrt(dx * dx + dy * dy);

            if (distance < SEPARATION_RADIUS) {
                force.x += dx / distance;
                force.y += dy / distance;
                count++;
            }
        }
    }

    if (count > 0) {
        force.x /= count;
        force.y /= count;
    }

    return force;
}

// Calculate the alignment force (match the velocity of nearby drones)
SDL_Point alignment(Drone *drone) {
    SDL_Point force = {0, 0};
    int count = 0;

    for (int i = 0; i < NUM_DRONES; i++) {
        if (&drones[i] != drone) {
            float dx = drone->x - drones[i].x;
            float dy = drone->y - drones[i].y;
            float distance = sqrt(dx * dx + dy * dy);

            if (distance < ALIGNMENT_RADIUS) {
                force.x += drones[i].vx;
                force.y += drones[i].vy;
                count++;
            }
        }
    }

    if (count > 0) {
        force.x /= count;
        force.y /= count;
    }

    return force;
}

// Calculate the cohesion force (move towards the average position of nearby drones)
SDL_Point cohesion(Drone *drone) {
    SDL_Point force = {0, 0};
    int count = 0;

    for (int i = 0; i < NUM_DRONES; i++) {
        if (&drones[i] != drone) {
            float dx = drone->x - drones[i].x;
            float dy = drone->y - drones[i].y;
            float distance = sqrt(dx * dx + dy * dy);

            if (distance < COHESION_RADIUS) {
                force.x += drones[i].x;
                force.y += drones[i].y;
                count++;
            }
        }
    }

    if (count > 0) {
        force.x /= count;
        force.y /= count;
        force.x -= drone->x;
        force.y -= drone->y;
    }

    return force;
}


// Render text on screen
void renderText(const char *text, int x, int y, SDL_Color color) {
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// Update drones based on Boid behavior
void updateDrones() {
    for (int i = 0; i < NUM_DRONES; i++) {
        float avg_vx = 0, avg_vy = 0;
        float avg_x = 0, avg_y = 0;
        float separation_x = 0, separation_y = 0;
        int neighbors = 0;

        for (int j = 0; j < NUM_DRONES; j++) {
            if (i == j) continue;
            float dx = drones[i].x - drones[j].x;
            float dy = drones[i].y - drones[j].y;
            float distance = sqrt(dx * dx + dy * dy);

            // Always include every other drone for cohesion and alignment:
            avg_x += drones[j].x;
            avg_y += drones[j].y;
            avg_vx += drones[j].vx;
            avg_vy += drones[j].vy;

            // Separation: only for drones closer than SEPARATION_RADIUS (now 25.0f)
            if (distance < SEPARATION_RADIUS) {
                separation_x += (drones[i].x - drones[j].x);
                separation_y += (drones[i].y - drones[j].y);
            }

            neighbors++;
        }

        if (neighbors > 0) {
            avg_x /= neighbors;
            avg_y /= neighbors;
            avg_vx /= neighbors;
            avg_vy /= neighbors;

            // Apply the forces with the new weights:
            drones[i].vx += (avg_x - drones[i].x) * COHESION_WEIGHT;
            drones[i].vy += (avg_y - drones[i].y) * COHESION_WEIGHT;

            drones[i].vx += avg_vx * ALIGNMENT_WEIGHT;
            drones[i].vy += avg_vy * ALIGNMENT_WEIGHT;

            drones[i].vx += separation_x * SEPARATION_WEIGHT;
            drones[i].vy += separation_y * SEPARATION_WEIGHT;
        }

        // (Optional) Remove friction to match reference exactly:
        // drones[i].vx *= FRICTION;
        // drones[i].vy *= FRICTION;

        // Limit the velocity
        float speed = sqrt(drones[i].vx * drones[i].vx + drones[i].vy * drones[i].vy);
        if (speed > MAX_SPEED) {
            drones[i].vx = (drones[i].vx / speed) * MAX_SPEED;
            drones[i].vy = (drones[i].vy / speed) * MAX_SPEED;
        }

        // Update position and wrap around
        drones[i].x += drones[i].vx;
        drones[i].y += drones[i].vy;
        if (drones[i].x < 0) drones[i].x = WIDTH;
        if (drones[i].x >= WIDTH) drones[i].x = 0;
        if (drones[i].y < 0) drones[i].y = HEIGHT;
        if (drones[i].y >= HEIGHT) drones[i].y = 0;

        if (drones[i].vx != 0 || drones[i].vy != 0) {
            float angle = atan2(drones[i].vy, drones[i].vx);
            drones[i].x += cos(angle) * 0.5; // Minor offset to align movement direction
            drones[i].y += sin(angle) * 0.5;
        }
    }
}

// Update player movement (mouse-based)
void updatePlayer(const Uint8 *keystate, int mouseX, int mouseY) {
    // Calculate direction vector toward the mouse
    float dx = mouseX - player.x;
    float dy = mouseY - player.y;

    // Normalize the vector
    float distance = sqrt(dx * dx + dy * dy);
    if (distance > 0) {
        dx /= distance;
        dy /= distance;
    }

    // Move player towards the mouse position
    player.vx += dx * PLAYER_ACCEL;
    player.vy += dy * PLAYER_ACCEL;

    // Apply friction
    player.vx *= FRICTION;
    player.vy *= FRICTION;

    // Update player position
    player.x += player.vx;
    player.y += player.vy;

    // Ensure player faces movement direction
    if (player.vx != 0 || player.vy != 0) {
        float angle = atan2(player.vy, player.vx);
        player.x += cos(angle) * 0.5; // Slight correction for rotation effect
        player.y += sin(angle) * 0.5;
    }

}

// Check for collisions
void checkCollisions() {
    for (int i = 0; i < NUM_DRONES; i++) {
        float dist = sqrt(pow(player.x - drones[i].x, 2) + pow(player.y - drones[i].y, 2));
        if (dist < 10) {
            playerHealth -= 1;
            if (playerHealth <= 0) {
                gameOver = 1;
                inGame = 0;
                if (score > highScore) {
                    highScore = score;
                    saveHighScore();
                }
            }
        }
    }
}

void checkCircleCollisions() {
    for (int i = 0; i < numCircles; i++) {
        if (circles[i].isVisible) {
            float dist = sqrt(pow(player.x - circles[i].x, 2) + pow(player.y - circles[i].y, 2));
            if (dist < circles[i].radius) {
                // Player passed over the circle
                circles[i].isVisible = 0;  // Circle disappears
                score += 10;  // Increase score

                // Remove the circle by shifting remaining circles down
                for (int j = i; j < numCircles - 1; j++) {
                    circles[j] = circles[j + 1];  // Move each circle down one slot
                }
                numCircles--;  // Decrease the number of active circles
                i--;  // Adjust index to avoid skipping a circle
            }
        }
    }
}

// Render game
void renderGame() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    renderBackground();

    if (rand() % 100 < 3) {  // 3% chance every frame
        spawnPlants();
    }    
    renderPlants();

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    // Rendering the opponent drones on-screen
    for (int i = 0; i < NUM_DRONES; i++) {
        drawBee(renderer, (int)drones[i].x, (int)drones[i].y, (float)drones[i].vx, (float)drones[i].vy,0);
    }

    // Rendering the player drone on-screen
    drawBee(renderer,(int)player.x,(int)player.y,(float)player.vx,(float)player.vy,1);

    // Determine the health bar color based on the health value
    SDL_Color healthColor;
    if (playerHealth >= 60) {
        healthColor = (SDL_Color){0, 255, 0, 255};  // Green for 60-100% health
    } else if (playerHealth >= 30) {
        healthColor = (SDL_Color){255, 255, 0, 255};  // Yellow for 30-59% health
    } else if (playerHealth >= 10) {
        healthColor = (SDL_Color){255, 165, 0, 255};  // Orange for 10-29% health
    } else {
        healthColor = (SDL_Color){255, 0, 0, 255};  // Red for below 10% health
    }
    
    // Render circles and handle fade-in/fade-out
    for (int i = 0; i < numCircles; i++) {
        if (circles[i].isVisible) {
            Uint32 currentTime = SDL_GetTicks();
            // Apply fade-in effect
            if (circles[i].alpha < 255 && currentTime - circles[i].lastAppearanceTime < 1000) {
                circles[i].alpha += 5;
            }

            // Apply fade-out effect after 5 seconds
            if (currentTime - circles[i].lastAppearanceTime > 5000 && circles[i].alpha > 0) {
                circles[i].alpha -= 5;
            }

            // Set the alpha value for the circle color
            SDL_SetRenderDrawColor(renderer, circles[i].color.r, circles[i].color.g, circles[i].color.b, circles[i].alpha);

            // Draw the circle (using filled circle rendering)
            SDL_RenderFillCircle(renderer, (int)circles[i].x, (int)circles[i].y, circles[i].radius);
        }
    }

    updateDrones();
    spawnCircles();
    checkCircleCollisions();

    // Set the health bar color and render it
    SDL_SetRenderDrawColor(renderer, healthColor.r, healthColor.g, healthColor.b, healthColor.a);
    SDL_Rect healthBar = {10, 10, playerHealth * 2, 20};  // Health bar width depends on health value
    SDL_RenderFillRect(renderer, &healthBar);

    // Render the numeric value of health at the center of the health bar
    char healthText[50];
    sprintf(healthText, "%d", playerHealth);  // Convert health to string with 2 decimal places
    int healthTextWidth, healthTextHeight;
    TTF_SizeText(font, healthText, &healthTextWidth, &healthTextHeight);

    // Render health in the middle of the health bar
    renderText(healthText, 10 + (playerHealth * 2 - healthTextWidth) / 2, 10 + (20 - healthTextHeight) / 2, (SDL_Color){0, 0, 0, 255});

    // Render score
    char scoreText[50];
    sprintf(scoreText, "Score: %d", score);
    renderText(scoreText, WIDTH - 150, 10, (SDL_Color){255, 255, 255, 255});

    SDL_RenderPresent(renderer);
    
}


// Render menu buttons
void renderButton(Button *button, const char *text) {
    SDL_Color color = button->clicked ? button->clickColor : (button->hovered ? button->hoverColor : button->defaultColor);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &button->rect);

    int textWidth, textHeight;
    TTF_SizeText(font, text, &textWidth, &textHeight);
    renderText(text, button->rect.x + (button->rect.w - textWidth) / 2, button->rect.y + (button->rect.h - textHeight) / 2, (SDL_Color){0, 0, 0, 255});
}

// Render menu
void renderMenu() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    renderText("DRONE ZONE", WIDTH / 2 - 100, 50, (SDL_Color){255, 255, 0, 255});
    renderButton(&playButton, "Play");
    renderButton(&helpButton, "Help");
    renderButton(&exitButton, "Exit");

    SDL_RenderPresent(renderer);
}

// Render the Help screen
void renderHelp() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    renderText("DRONE ZONE (c) Dewan Mukto, MuxAI 2025", WIDTH / 2 - 220, 50, (SDL_Color){255, 255, 255, 255});
    renderText("dewanmukto.mux8.com", WIDTH / 2 - 100, 100, (SDL_Color){255, 255, 255, 55});
    renderText("Use your mouse for moving your bee.", WIDTH / 2 - 220, 150, (SDL_Color){255, 255, 255, 255});
    renderText("Avoid crashing into other bees!", WIDTH / 2 - 220, 200, (SDL_Color){255, 255, 255, 255});

    // Render the Back button
    renderButton(&backButton, "Back");

    SDL_RenderPresent(renderer);
}

void renderGameOver() {
    // Transition: slowly turn the background gray
    static int grayProgress = 0;
    if (grayProgress < 255) {
        grayProgress += 1;  // Gradually turn to gray
    }

    SDL_SetRenderDrawColor(renderer, grayProgress, grayProgress, grayProgress, 255);
    SDL_RenderClear(renderer); // Clear with the current gray value

    // Render the game over screen (black centered rectangle)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Black rectangle
    SDL_Rect gameOverRect = {WIDTH / 2 - 150, HEIGHT / 2 - 100, 300, 200}; // Centered
    SDL_RenderFillRect(renderer, &gameOverRect);

    // Display current score and high score
    char scoreText[100];
    sprintf(scoreText, "Score: %d", score);
    SDL_Color scoreColor = (score == highScore) ? (SDL_Color){255, 255, 0, 255} : (SDL_Color){255, 255, 255, 255};
    renderText(scoreText, WIDTH / 2 - 100, HEIGHT / 2 - 50, scoreColor);

    char highScoreText[100];
    sprintf(highScoreText, "High Score: %d", highScore);
    renderText(highScoreText, WIDTH / 2 - 100, HEIGHT / 2, (SDL_Color){255, 255, 255, 255});

    renderButton(&backButton, "Retry");
    renderButton(&menuButton, "Menu");

    SDL_RenderPresent(renderer);
}


void handleGameOverEvents(SDL_Event *e) {
    int x, y;
    SDL_GetMouseState(&x, &y);

    // Check the Retry button (backButton)
    backButton.hovered = (x >= backButton.rect.x && x <= backButton.rect.x + backButton.rect.w &&
                          y >= backButton.rect.y && y <= backButton.rect.y + backButton.rect.h);
    
    // Check the Menu button (menuButton)
    menuButton.hovered = (x >= menuButton.rect.x && x <= menuButton.rect.x + menuButton.rect.w &&
                          y >= menuButton.rect.y && y <= menuButton.rect.y + menuButton.rect.h);

    if (e->type == SDL_MOUSEBUTTONDOWN && backButton.hovered) {
        backButton.clicked = 1;
    } else if (e->type == SDL_MOUSEBUTTONUP && backButton.clicked) {
        backButton.clicked = 0;
        // Retry: reset game state
        inGame = 1;
        inHelp = 0;
        gameOver = 0;
        playerHealth = 100;
        score = 0;
        initDrones();
        numCircles = 0;
        // Reset gray progress for a fresh transition next time
        // (Either reinitialize grayProgress here or in initDrones())
    }

    if (e->type == SDL_MOUSEBUTTONDOWN && menuButton.hovered) {
        menuButton.clicked = 1;
    } else if (e->type == SDL_MOUSEBUTTONUP && menuButton.clicked) {
        menuButton.clicked = 0;
        // Menu: go back to main menu
        inGame = 0;     // Set game state off so that the menu shows
        gameOver = 0;   // Clear game over flag
        // Optionally, reset other game state if needed:
        initDrones();
        numCircles = 0;
        grayProgress = 0;
    }
}


// Modify handleMenuEvents
void handleMenuEvents(SDL_Event *e) {
    int x, y;
    SDL_GetMouseState(&x, &y);

    Button *buttons[] = {&playButton, &helpButton, &exitButton};

    for (int i = 0; i < 3; i++) {
        Button *button = buttons[i];
        button->hovered = (x >= button->rect.x && x <= button->rect.x + button->rect.w &&
                           y >= button->rect.y && y <= button->rect.y + button->rect.h);

        if (e->type == SDL_MOUSEBUTTONDOWN && button->hovered) {
            button->clicked = 1;
        } else if (e->type == SDL_MOUSEBUTTONUP && button->clicked) {
            button->clicked = 0;
            if (button == &playButton) {
                inGame = 1;
                inHelp = 0;  // Ensure we're not in help mode
                initDrones();
            } else if (button == &exitButton) {
                running = 0;
            } else if (button == &helpButton) {
                inHelp = 1;  // Switch to the help screen
                inGame = 0;  // Ensure we're not in the game mode
            }
        }
    }

    // Handle the back button on the help screen
    if (inHelp) {
        backButton.hovered = (x >= backButton.rect.x && x <= backButton.rect.x + backButton.rect.w &&
                              y >= backButton.rect.y && y <= backButton.rect.y + backButton.rect.h);
        if (e->type == SDL_MOUSEBUTTONDOWN && backButton.hovered) {
            backButton.clicked = 1;
        } else if (e->type == SDL_MOUSEBUTTONUP && backButton.clicked) {
            backButton.clicked = 0;
            inHelp = 0;  // Go back to the main menu
        }
    }

    // Handle events for game over screen retry button
    if (!inGame && !inHelp) {
        handleGameOverEvents(e);
    }
}

// Main loop
int main() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    window = SDL_CreateWindow("Drone Zone", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    font = TTF_OpenFont("Sigmar-Regular.ttf", 24);

    loadHighScore();
    renderMenu();

    Uint32 frameStart; // Variable to track the start of each frame

    while (running) {
        frameStart = SDL_GetTicks(); // Get the current time at the start of the frame

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            handleMenuEvents(&e);
        }

        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        if (inHelp) {
            renderHelp(); // Help screen
        } else if (gameOver) {
            renderGameOver(); // Game over screen
        } else if (!inGame) {
            renderMenu(); // Main menu
        } else if (inGame) {
            updatePlayer(NULL, mouseX, mouseY);
            checkCollisions();
            renderGame(); // In-game rendering
        }        

        // Calculate how long the frame took
        Uint32 frameTime = SDL_GetTicks() - frameStart;

        // If the frame took less than the frame delay, delay the remaining time
        if (frameTime < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - frameTime);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
