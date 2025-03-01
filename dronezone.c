#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WIDTH 800
#define HEIGHT 600
#define MAX_SPEED 4
#define NUM_DRONES 20
#define PLAYER_ACCEL 0.1f
#define FRICTION 0.98f
#define FPS 60
#define FRAME_DELAY (1000 / FPS)
#define SEPARATION_RADIUS 50.0f
#define ALIGNMENT_RADIUS 100.0f
#define COHESION_RADIUS 100.0f
#define SEPARATION_WEIGHT 1.5f
#define ALIGNMENT_WEIGHT 1.0f
#define COHESION_WEIGHT 1.0f

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

Drone player;
Drone drones[NUM_DRONES];

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;

int running = 1;
int inGame = 0;
int playerHealth = 100;
int score = 0;
int highScore = 0;

// Menu Buttons
Button playButton = {{WIDTH / 2 - 50, 200, 100, 40}, {255, 255, 255, 255}, {200, 200, 200, 255}, {100, 100, 100, 255}, 0, 0};
Button helpButton = {{WIDTH / 2 - 50, 250, 100, 40}, {255, 255, 255, 255}, {200, 200, 200, 255}, {100, 100, 100, 255}, 0, 0};
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

// Initialize drones
void initDrones() {
    srand(time(NULL));
    player.x = WIDTH / 2;
    player.y = HEIGHT / 2;
    player.vx = player.vy = 0;

    // Set player health to 100 at the start of the game
    playerHealth = 100; 

    for (int i = 0; i < NUM_DRONES; i++) {
        drones[i].x = rand() % WIDTH;
        drones[i].y = rand() % HEIGHT;
        drones[i].vx = (float)(rand() % MAX_SPEED) - MAX_SPEED / 2;
        drones[i].vy = (float)(rand() % MAX_SPEED) - MAX_SPEED / 2;
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
        SDL_Point sep = separation(&drones[i]);
        SDL_Point ali = alignment(&drones[i]);
        SDL_Point coh = cohesion(&drones[i]);

        // Apply the weights to each force
        drones[i].vx += sep.x * SEPARATION_WEIGHT + ali.x * ALIGNMENT_WEIGHT + coh.x * COHESION_WEIGHT;
        drones[i].vy += sep.y * SEPARATION_WEIGHT + ali.y * ALIGNMENT_WEIGHT + coh.y * COHESION_WEIGHT;

        // Apply some friction to reduce speed over time
        drones[i].vx *= FRICTION;
        drones[i].vy *= FRICTION;

        // Update drone position
        drones[i].x += drones[i].vx;
        drones[i].y += drones[i].vy;

        // Prevent drones from going out of bounds (wrap around screen)
        if (drones[i].x < 0) drones[i].x = WIDTH;
        if (drones[i].x >= WIDTH) drones[i].x = 0;
        if (drones[i].y < 0) drones[i].y = HEIGHT;
        if (drones[i].y >= HEIGHT) drones[i].y = 0;
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
}

// Check for collisions
void checkCollisions() {
    for (int i = 0; i < NUM_DRONES; i++) {
        float dist = sqrt(pow(player.x - drones[i].x, 2) + pow(player.y - drones[i].y, 2));
        if (dist < 10) {
            playerHealth -= 1;
            if (playerHealth <= 0) {
                inGame = 0;
                if (score > highScore) {
                    highScore = score;
                    saveHighScore();
                }
            }
        }
    }
}

// Render game
void renderGame() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < NUM_DRONES; i++) {
        SDL_Rect r = {(int)drones[i].x, (int)drones[i].y, 5, 5};
        SDL_RenderFillRect(renderer, &r);
    }

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect p = {(int)player.x, (int)player.y, 8, 8};
    SDL_RenderFillRect(renderer, &p);

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
    
    updateDrones();

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

// Handle menu events
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
                initDrones();
            } else if (button == &exitButton) {
                running = 0;
            }
        }
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

        if (!inGame) renderMenu();
        else {
            updatePlayer(NULL, mouseX, mouseY);
            checkCollisions();
            renderGame();
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
