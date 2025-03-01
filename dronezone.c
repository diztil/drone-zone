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

    for (int i = 0; i < NUM_DRONES; i++) {
        drones[i].x = rand() % WIDTH;
        drones[i].y = rand() % HEIGHT;
        drones[i].vx = (float)(rand() % MAX_SPEED) - MAX_SPEED / 2;
        drones[i].vy = (float)(rand() % MAX_SPEED) - MAX_SPEED / 2;
    }
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

// Update player movement
void updatePlayer(const Uint8 *keystate) {
    if (keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP]) player.vy -= PLAYER_ACCEL;
    if (keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN]) player.vy += PLAYER_ACCEL;
    if (keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_LEFT]) player.vx -= PLAYER_ACCEL;
    if (keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_RIGHT]) player.vx += PLAYER_ACCEL;

    player.vx *= FRICTION;
    player.vy *= FRICTION;
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

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_Rect healthBar = {10, 10, playerHealth * 2, 20};
    SDL_RenderFillRect(renderer, &healthBar);

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

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            handleMenuEvents(&e);
        }
        if (!inGame) renderMenu();
        else renderGame();
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
