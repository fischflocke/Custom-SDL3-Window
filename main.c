/* 
  A fully functional SDL3 window with a custom-drawn non-client area and shadow 
  Copyright (C) 2025 fischflocke

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// Standard includes
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

// SDL3 includes
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

// Local includes
#include "shadow.h"

bool initSDL(void);
bool createWindow(void);
void destroyWindow(void);
void handleEvent(const SDL_Event *event);
SDL_HitTestResult hitTest(SDL_Window *win, const SDL_Point *area, void *data);
void drawWindow(void);
void drawShadow(void);
void loadImageResources(void);
void updateLayout(void);

SDL_Window *wnd = NULL;
SDL_Renderer *rnd = NULL;
bool appShouldExit = false;
bool windowShouldBeRedrawn = true;

struct {
    SDL_FRect window;
    SDL_FRect background;
    SDL_FRect titleBar;
    SDL_FRect clientArea;
    float scale;
} layout;

struct {
    SDL_Texture *bottom;
    SDL_Texture *corner;
    SDL_Texture *left;
} shadow;

typedef struct {
    SDL_Color border;
    SDL_Color background;
    SDL_Color titleBar;
} palette;

struct {
    palette light;
    palette dark;
    bool useLight;
} theme = {
    .light = {{200, 200, 200, 255}, {227, 227, 227, 255}, {255, 255, 255, 255}},
    .dark = {{55, 55, 55, 255}, {27, 27, 27, 255}, {0, 0, 0, 255}},
    .useLight = true
};

int main(void) {
    // Init SDL and create window and renderer
    if (!initSDL())
        return EXIT_FAILURE;
    if (!createWindow())
        return EXIT_FAILURE;

    // Register hit test
    if (!SDL_SetWindowHitTest(wnd, hitTest, NULL)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to enable hit tests",
                                 SDL_GetError(), NULL);
        return EXIT_FAILURE;
    }

    // Load image resources
    loadImageResources();
    // Update the window's layout
    updateLayout();
    // Check if dark mode is enabled
    theme.useLight = SDL_GetSystemTheme() != SDL_SYSTEM_THEME_DARK;

    // Main update loop
    SDL_Event event;
    do {
        // Redraw window if needed
        if (windowShouldBeRedrawn) {
            drawWindow();
            windowShouldBeRedrawn = false;
        }

        // Wait for unhandled events and handle them
        /* We need the timeout because otherwise, the app would only react to changes in the system
         * theme after receiving input, such as mouse movement. Unlike a loop that constantly polls
         * for unhandled events, this method does not cause a permanent CPU load. */
        SDL_WaitEventTimeout(&event, 100);
        handleEvent(&event);
    } while (!appShouldExit);

    // Clean up and exit
    return EXIT_SUCCESS;
}

bool initSDL(void) {
    // Init SDL3
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to initialize SDL3",
                                 SDL_GetError(), NULL);
        return false;
    }
    atexit(SDL_Quit);

    // Init SDL3_ttf
    if (!TTF_Init()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                 "Failed to initialize SDL3_ttf", SDL_GetError(),
                                 NULL);
        return false;
    }
    atexit(TTF_Quit);

    return true;
}

bool createWindow(void) {
    // Create window
    wnd = SDL_CreateWindow("Demo Window", 800, 600,
                           SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE |
                           SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT |
                           SDL_WINDOW_INPUT_FOCUS);
    if (!wnd) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to create window",
                                 SDL_GetError(), NULL);
        return false;
    }
    atexit(destroyWindow);

    // Set minimum size
    SDL_SetWindowMinimumSize(wnd, 126, 126);

    // Create renderer
    rnd = SDL_CreateRenderer(wnd, NULL);
    if (!rnd) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to create renderer",
                                 SDL_GetError(), NULL);
        return false;
    }
    // rnd will be destroyed by destroyWindow

    return true;
}

void destroyWindow(void) {
    // Destroy renderer
    if (rnd) {
        SDL_DestroyRenderer(rnd);
        rnd = NULL;
    }

    // Destroy window
    if (wnd) {
        SDL_DestroyWindow(wnd);
        wnd = NULL;
    }
}

void handleEvent(const SDL_Event *event) {
    switch (event->type) {
    case SDL_EVENT_QUIT:
        appShouldExit = true;
        break;
    case SDL_EVENT_WINDOW_EXPOSED:
        windowShouldBeRedrawn = true;
        break;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        updateLayout();
        break;
    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        updateLayout();
        break;
    case SDL_EVENT_SYSTEM_THEME_CHANGED:
        theme.useLight = SDL_GetSystemTheme() != SDL_SYSTEM_THEME_DARK;
        windowShouldBeRedrawn = true;
        break;
    }
}

SDL_HitTestResult hitTest(SDL_Window *win, const SDL_Point *area, void *data) {
    float scale = layout.scale;

    // Shortcut for background position and size
    float bx = layout.background.x, by = layout.background.y,
            bw = layout.background.w, bh = layout.background.h;
    // Cursor position as float
    SDL_FPoint pos = {area->x * scale, area->y * scale};
    // Cursor position as int
    int x = pos.x, y = pos.y;

    // Tolerances
    int edgeTol = ceilf(2 * scale), cornerTol = ceilf(8 * scale);

    // Left border
    if (x >= bx - edgeTol && x <= bx + edgeTol) {
        if (y < by + cornerTol)
            return SDL_HITTEST_RESIZE_TOPLEFT;
        else if (y >= by + bh - cornerTol)
            return SDL_HITTEST_RESIZE_BOTTOMLEFT;
        return SDL_HITTEST_RESIZE_LEFT;
    }

    // Right border
    else if (x >= bx + bw - edgeTol && x <= bx + bw + edgeTol) {
        if (y < by + cornerTol)
            return SDL_HITTEST_RESIZE_TOPRIGHT;
        else if (y >= by + bh - cornerTol)
            return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
        return SDL_HITTEST_RESIZE_RIGHT;
    }

    // Top border
    else if (y >= by - edgeTol && y <= by + edgeTol) {
        return SDL_HITTEST_RESIZE_TOP;
    }

    // Bottom border
    else if (y >= by + bh - edgeTol && y <= by + bh + edgeTol) {
        return SDL_HITTEST_RESIZE_BOTTOM;
    }

    // Title bar
    else if (SDL_PointInRectFloat(&pos, &layout.titleBar)) {
        return SDL_HITTEST_DRAGGABLE;
    }

    return SDL_HITTEST_NORMAL;
}

void drawWindow(void) {
    // Clear with transparent black
    SDL_SetRenderDrawColor(rnd, 0, 0, 0, 0);
    SDL_RenderClear(rnd);

    // Draw shadow
    drawShadow();

    // Draw background border and client area
    SDL_Color c;

    c = theme.useLight ? theme.light.border : theme.dark.border;
    SDL_SetRenderDrawColor(rnd, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(rnd, &layout.background);

    c = theme.useLight ? theme.light.titleBar : theme.dark.titleBar;
    SDL_SetRenderDrawColor(rnd, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(rnd, &layout.titleBar);

    c = theme.useLight ? theme.light.background : theme.dark.background;
    SDL_SetRenderDrawColor(rnd, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(rnd, &layout.clientArea);

    // Swap buffers
    SDL_RenderPresent(rnd);
}

void drawShadow(void) {
    int w = layout.window.w, h = layout.window.h;

    SDL_FRect dest = {0, 0, 55, 55};

    // Corners

    // Top Left
    SDL_RenderTextureRotated(rnd, shadow.corner, NULL, &dest, 0, NULL,
                             SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
    // Top right
    dest.x = w - dest.w;
    SDL_RenderTextureRotated(rnd, shadow.corner, NULL, &dest, 0, NULL,
                             SDL_FLIP_VERTICAL);
    // Bottom right
    dest.y = h - dest.h;
    SDL_RenderTexture(rnd, shadow.corner, NULL, &dest);
    // Bottom left
    dest.x = 0;
    SDL_RenderTextureRotated(rnd, shadow.corner, NULL, &dest, 0, NULL,
                             SDL_FLIP_HORIZONTAL);

    // Sides

    // Top
    dest.x = 55;
    dest.y = 0;
    dest.w = w - 110;
    dest.h = 16;
    SDL_RenderTextureRotated(rnd, shadow.bottom, NULL, &dest, 0, NULL,
                             SDL_FLIP_VERTICAL);

    // Bottom
    dest.y = h - 16;
    SDL_RenderTexture(rnd, shadow.bottom, NULL, &dest);

    // Left
    dest.x = 0;
    dest.y = 55;
    dest.w = 16;
    dest.h = h - 110;
    SDL_RenderTextureRotated(rnd, shadow.left, NULL, &dest, 0, NULL,
                             SDL_FLIP_NONE);

    // Right
    dest.x = w - 16;
    SDL_RenderTextureRotated(rnd, shadow.left, NULL, &dest, 0, NULL,
                             SDL_FLIP_HORIZONTAL);
}

void loadImageResources(void) {
    // Load shadow textures
    shadow.corner = IMG_LoadTextureTyped_IO(
                rnd, SDL_IOFromMem(corner_png, corner_png_len), true, "png");
    shadow.bottom = IMG_LoadTextureTyped_IO(
                rnd, SDL_IOFromMem(bottom_png, bottom_png_len), true, "png");
    shadow.left = IMG_LoadTextureTyped_IO(
                rnd, SDL_IOFromMem(left_png, left_png_len), true, "png");

    // Set shadow intensity
    SDL_SetTextureAlphaModFloat(shadow.corner, 0.3);
    SDL_SetTextureAlphaModFloat(shadow.bottom, 0.3);
    SDL_SetTextureAlphaModFloat(shadow.left, 0.3);
}

void updateLayout(void) {
    // Get content scale
    float scale = SDL_GetWindowDisplayScale(wnd);
    layout.scale = scale;

    // Update window size
    int w, h;
    SDL_GetWindowSizeInPixels(wnd, &w, &h);
    layout.window.x = 0;
    layout.window.y = 0;
    layout.window.w = w;
    layout.window.h = h;

    // Update background area without shadows
    layout.background.x = shadow.left->w;
    layout.background.y = shadow.bottom->h;
    layout.background.w = w - 2 * shadow.left->w;
    layout.background.h = h - 2 * shadow.bottom->h;

    // Remaining height for client area
    float rh = layout.background.h - 2 * floorf(scale);

    // Update title area
    layout.titleBar.x = layout.background.x + floorf(scale);
    layout.titleBar.y = layout.background.y + floorf(scale);
    layout.titleBar.w = layout.background.w - 2 * floorf(scale);
    layout.titleBar.h = ceilf(30 * scale);

    rh -= (layout.titleBar.h + floorf(scale));

    // Update client area
    layout.clientArea.x = layout.titleBar.x;
    layout.clientArea.y = layout.titleBar.y + layout.titleBar.h + floorf(scale);
    layout.clientArea.w = layout.titleBar.w;
    layout.clientArea.h = rh;

    // Mark window as dirty
    windowShouldBeRedrawn = true;
}
