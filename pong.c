#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_stdinc.h>
#include <stdio.h>
#include <math.h>

// #include "app.h"

typedef struct {
  float x, y;
} Vec2;

Vec2 add(Vec2 a, Vec2 b) {
  return (Vec2){a.x+b.x, a.y+b.y};
}
Vec2 scale(Vec2 a, float s) {
  return (Vec2){a.x*s, a.y*s};
}

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;

  int WIDTH;
  int HEIGHT;

  float margin;

  float paddle_speed;
  float paddle_width;
  float paddle_thickness;

  int s1, s2;
  float p1, p2;
  float p1d, p2d;
  
  float ball_speed;
  Vec2 ball;
  Vec2 ball_dir;
  
  int spawns;

  Uint64 last, now;
  float deltaT;
} AppState;

void initialise_appstate(AppState *app) {
  app->WIDTH = 1080;
  app->HEIGHT = 720;

  app->margin = 15;

  app->paddle_speed = 25;
  app->paddle_width = 80;
  app->paddle_thickness = 5;
  
  app->s1 = app->s2 = 0;
  app->p1 = app->p2 = app->HEIGHT/2;
  
  app->ball_speed = 35;
  
  app->spawns = 0;
  app->last = SDL_GetPerformanceCounter();
}


// #include "sim.h"

void reset_ball(AppState *app) {
  app->ball.x = app->WIDTH/2;
  app->ball.y = app->HEIGHT/2;

  app->spawns++;
  const float GOLDEN_ANGLE = 2.39996322973;
  app->ball_dir.x = cosf(app->spawns * GOLDEN_ANGLE);
  app->ball_dir.y = sinf(app->spawns * GOLDEN_ANGLE);

}

void inc_score(AppState *app, int player) {
  switch (player) {
  case 1:
    app->s1++;
    break;
  case 2:
    app->s2++;
    break;
  default:
    break;
  }
}

void update_ball(AppState *app) {
  app->ball = add(app->ball, scale(app->ball_dir, app->deltaT * app->ball_speed));

  // off left edge
  if (app->ball.x < 0) {
    inc_score(app, 2);
    reset_ball(app);
  }
  // hit left paddle
  if (app->ball.x < (app->margin + app->paddle_thickness) 
      && app->ball.x > app->margin 
      && abs(app->ball.y - app->p1) < app->paddle_width / 2) {
    app->ball_dir.x *= -1;
    app->ball.x = app->margin + app->paddle_thickness;
  }
  // hit right paddle
  if (app->ball.x > app->WIDTH - (app->margin + app->paddle_thickness) 
      && app->ball.x < app->WIDTH - app->margin 
      && abs(app->ball.y - app->p2) < app->paddle_width / 2) {
    app->ball_dir.x *= -1;
    app->ball.x = app->WIDTH - (app->margin + app->paddle_thickness);
  }
  // hit right edge
  if (app->ball.x > app->WIDTH) {
    inc_score(app, 1);
    reset_ball(app);
  }
  // hit top edge
  if (app->ball.y < 0) {
    app->ball.y *= -1;
    app->ball_dir.y *= -1;
  }
  // hit bottom edge
  if (app->ball.y > app->HEIGHT) {
    app->ball.y = 2 * app->HEIGHT - app->ball.y;
    app->ball_dir.y *= -1;
  }
}

void update_paddles(AppState *app) {
  app->p1 += app->p1d * app->deltaT;
  app->p2 += app->p2d * app->deltaT;
}

// #include "render.h"

void render_players(AppState *app) {
  SDL_SetRenderDrawColor(app->renderer, 220, 235, 255, SDL_ALPHA_OPAQUE);

  SDL_FRect pong_rect = (SDL_FRect) {
    app->margin, app->p1 - app->paddle_width/2,
    app->paddle_thickness, app->paddle_width
  };
  SDL_RenderFillRect(app->renderer, &pong_rect);
  
  pong_rect.x = app->WIDTH - app->margin - app->paddle_thickness;
  pong_rect.y = app->p2 - app->paddle_width/2;
  SDL_RenderFillRect(app->renderer, &pong_rect);
}

void render_ball(AppState *app) {
  const int num_verts = 16;
  SDL_Vertex vertices[3*num_verts] = {};
  const float TWO_PI = 6.28318530718f;
  const float radius = 6;

  float oldcos = radius, oldsin = 0;
  for (int i = 1; i <= num_verts; i++) {
    SDL_FColor colour = (SDL_FColor) {255, 235, 220, SDL_ALPHA_OPAQUE};

    float cos_kx = cosf(i * TWO_PI / num_verts) * radius;
    float sin_kx = sinf(i * TWO_PI / num_verts) * radius;

    vertices[3*i-3] = (SDL_Vertex) {{app->ball.x +      0, app->ball.y -      0}, colour, {0,0}}; // centre
    vertices[3*i-2] = (SDL_Vertex) {{app->ball.x + oldsin, app->ball.y - oldcos}, colour, {0,0}}; // first
    vertices[3*i-1] = (SDL_Vertex) {{app->ball.x + sin_kx, app->ball.y - cos_kx}, colour, {0,0}}; // second

    oldcos = cos_kx;
    oldsin = sin_kx;
  }

  SDL_RenderGeometry(app->renderer, NULL, vertices, 3*num_verts, NULL, 0);
}

char *text_buffer;
const int max_text = 140;

void render_score(AppState *app) {
  int length = snprintf(text_buffer, max_text, "Player 1: %d", app->s1);
  SDL_RenderDebugText(app->renderer, 1, 1, text_buffer);
  
  length = snprintf(text_buffer, max_text, "Player 2: %d", app->s2);
  SDL_RenderDebugText(app->renderer, app->WIDTH - 8*length, 1, text_buffer);
}

void render_frame(AppState *app) {
  SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(app->renderer, 26, 26, 26, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(app->renderer);

  render_players(app);
  render_ball(app);
  render_score(app);

  SDL_RenderPresent(app->renderer);
}



SDL_AppResult event_keydown(AppState *app, SDL_KeyboardEvent *event) {
  switch (event->key) {
  case SDLK_ESCAPE:
    return SDL_APP_SUCCESS;

  case SDLK_UP:
    app->p1d = -app->paddle_speed;
    break;

  case SDLK_DOWN:
    app->p1d = +app->paddle_speed;
    break;

  case SDLK_W:
    app->p2d = -app->paddle_speed;
    break;

  case SDLK_S:
    app->p2d = +app->paddle_speed;
    break;

  case SDLK_SPACE:
    reset_ball(app);
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult event_keyup(AppState *app, SDL_KeyboardEvent *event) {
  switch (event->key) {

  case SDLK_UP:
    app->p1d = 0;
    break;

  case SDLK_DOWN:
    app->p1d = 0;
    break;

  case SDLK_W:
    app->p2d = 0;
    break;

  case SDLK_S:
    app->p2d = 0;
    break;
  }
  return SDL_APP_CONTINUE;
}

// #include "colour.h"

const char* title = "Pong";

SDL_AppResult SDL_AppInit (void **appstate, int argc, char *argv[]) {
  SDL_SetAppMetadata(title, NULL, NULL);
  AppState *app = SDL_malloc(sizeof *app);
  *appstate = app;

  initialise_appstate(app);

  text_buffer = SDL_malloc(max_text);
  
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    SDL_Log("Failed to initialise SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  
  if (!SDL_CreateWindowAndRenderer(title, app->WIDTH, app->HEIGHT, 0, &app->window, &app->renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }
  
  reset_ball(app);

  app->last = SDL_GetPerformanceCounter();

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  AppState *app = appstate;

  switch (event->type) {

  case SDL_EVENT_QUIT:
    return SDL_APP_SUCCESS;

  case SDL_EVENT_KEY_DOWN:
    return event_keydown(app, &event->key);

  case SDL_EVENT_KEY_UP:
    return event_keyup(app, &event->key);

/*
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    return event_mousedown(app, &event->button);

  case SDL_EVENT_MOUSE_BUTTON_UP:
    return event_mouseup(app, &event->button);

  case SDL_EVENT_MOUSE_WHEEL:
    return event_mousewheel(app, &event->wheel);
*/
  default:
    return SDL_APP_CONTINUE;
  }
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  AppState *app = appstate;

  app->now = SDL_GetPerformanceCounter();
  app->deltaT = 10 * (app->now - app->last) / (float)SDL_GetPerformanceFrequency();
  app->last = app->now;

  update_paddles(app);
  update_ball(app);
  render_frame(app);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  AppState *app = appstate;

  SDL_free(app);
  SDL_free(text_buffer);
}