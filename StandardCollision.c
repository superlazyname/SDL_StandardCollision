#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

// Types
//---------------------------------------------------------------------------------------------------

typedef struct InitSDLValues_s
{
    SDL_Window*  Window;
    SDL_Renderer* Renderer;
    
} InitSDLValues_t;

typedef struct IntVec2
{
    int X; // X coordinate or column number
    int Y; // Y coordinate or row number
} IntVec2_t;

typedef struct Input_s
{
    int Quit;
    int ToggleBoundingBoxRender;
} Input;

// Constants
//---------------------------------------------------------------------------------------------------

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define BOUNDING_BOXES 8000

// change this to change the size of the window
const IntVec2_t cScreenResolution = {640, 480};

// this is more FYI than anything
const int cFPS = 60;

// 1/60 is 0.016666666666666666
const int cFrameDuration_ms = 16;

SDL_Rect BoundingBoxes[BOUNDING_BOXES];

const IntVec2_t cPlayerSize = {2, 2};

// Globals
//---------------------------------------------------------------------------------------------------

// various SDL resources required for rendering.
InitSDLValues_t SDLGlobals;

IntVec2_t MousePosition = {0,0};
int m_DrawBoundingBoxes = 0;


// Functions
//---------------------------------------------------------------------------------------------------

void GenerateCollisionReactangles(void)
{
    int boundingBoxIndex = 0;
    const IntVec2_t cBoundingBoxSize = {2, 2};

    IntVec2_t position = {0, 0};

    // space between bounding boxes
    const IntVec2_t cPadding = {4,4};

    for(boundingBoxIndex = 0; boundingBoxIndex < BOUNDING_BOXES; boundingBoxIndex++)
    {
        SDL_Rect boundingBox = {position.X, position.Y, cBoundingBoxSize.X, cBoundingBoxSize.Y};
        BoundingBoxes[boundingBoxIndex] = boundingBox;

        const int nextX = position.X + cBoundingBoxSize.X + cPadding.X;
        const int nextXExtent = nextX + cBoundingBoxSize.X;

        IntVec2_t newPosition = {0,0};

        if(nextXExtent > cScreenResolution.X)
        {
            newPosition.X = 0;
            newPosition.Y = position.Y + cBoundingBoxSize.Y + cPadding.Y;
        }
        else
        {
            newPosition.X = nextX;
            newPosition.Y = position.Y;
        }

        position = newPosition;
        
    }
}


const InitSDLValues_t InitSDL(IntVec2_t windowSize_px)
{
    InitSDLValues_t sdlInitResult = {NULL, NULL};

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) 
    {
        return sdlInitResult;
    }

    // Init the window
    SDL_Window* const window = SDL_CreateWindow("Bounding box collision", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowSize_px.X, windowSize_px.Y, SDL_WINDOW_SHOWN);
    if (!window) 
    {
        printf("An error occured while trying to create window : %s\n", SDL_GetError());
        return sdlInitResult;
    }

    // Init the renderer
    SDL_Renderer* const renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) 
    {
        printf("An error occured while trying to create renderer : %s\n", SDL_GetError());
        return sdlInitResult;
    }

    sdlInitResult.Window = window;
    sdlInitResult.Renderer = renderer;
    return sdlInitResult;
}

Input HandleInput(void)
{
    SDL_Event event = {0};
    Input input = {0};

    while (SDL_PollEvent(&event)) 
    {
        // E.g., from hitting the close window button
        if (event.type == SDL_QUIT) 
        {
            input.Quit = 1;
        }
        else if(event.type == SDL_MOUSEMOTION)
        {
            MousePosition.X = event.motion.x;
            MousePosition.Y = event.motion.y;
        }
        else if(event.type == SDL_KEYDOWN)
        {
            SDL_Keycode key = event.key.keysym.sym;

            if(event.key.keysym.scancode == SDL_SCANCODE_B)
            {
                input.ToggleBoundingBoxRender = 1;
            }
        }
    }

    return input;
}

int PointInRect(const IntVec2_t point, const IntVec2_t rectTopLeft, const IntVec2_t rectSize)
{
    // you might be wondering what the point of IntVect2 is if SDL_Point exists,
    // well, there isn't one, I just didn't know SDL already had something.
    SDL_Rect rect;
    rect.x = rectTopLeft.X;
    rect.y = rectTopLeft.Y;
    rect.w = rectSize.X;
    rect.h = rectSize.Y;

    SDL_Point sdlPoint;
    sdlPoint.x = point.X;
    sdlPoint.y = point.Y;

    SDL_bool inRect = SDL_PointInRect(&sdlPoint, &rect);

    int bInRect = (inRect == SDL_TRUE);

    //printf("Point (%d, %d) in rect [(%d, %d), (%d, %d)]: %d (b) %d\n", point.X, point.Y, rect.x, rect.y, rect.w, rect.h, inRect, bInRect);

    return bInRect;
}


int HasCollided(const IntVec2_t playerPosition)
{
    const Uint32 beforeCollisionCheck = SDL_GetTicks();
    int boundingBoxIndex = 0;

    const SDL_Rect playerRectangle = {playerPosition.X, playerPosition.Y, cPlayerSize.X, cPlayerSize.Y};

    int collided = 0;

    for(boundingBoxIndex = 0; boundingBoxIndex < ARRAY_SIZE(BoundingBoxes); boundingBoxIndex++)
    {
        const SDL_bool hasIntersection = SDL_HasIntersection(&playerRectangle, &BoundingBoxes[boundingBoxIndex]);

        if(hasIntersection == SDL_TRUE)
        {
            // note: could exit here, but I'm not going to, to benchmark the worst case
            collided = 1;
        }
    }

    const Uint32 afterCollisionCheck = SDL_GetTicks();

    // this does not become greater than 0 until there are over 200k bounding boxes
    const Uint32 collisionTime = afterCollisionCheck - beforeCollisionCheck;

    return collided;
}

void DrawBoundingBoxes(void)
{
    int boundingBoxIndex = 0;

    SDL_SetRenderDrawColor(SDLGlobals.Renderer, 255, 0, 0, 255);

    for(boundingBoxIndex = 0; boundingBoxIndex < ARRAY_SIZE(BoundingBoxes); boundingBoxIndex++)
    {
        SDL_RenderDrawRect(SDLGlobals.Renderer, &BoundingBoxes[boundingBoxIndex]);
    }
}



void Render(const int didCollide, const int drawBoundingBoxes)
{
    if(didCollide)
    {
        SDL_SetRenderDrawColor(SDLGlobals.Renderer, 100, 0, 0, 255);
    }
    else
    {
        SDL_SetRenderDrawColor(SDLGlobals.Renderer, 0, 0, 100, 255);
    }

    SDL_RenderClear(SDLGlobals.Renderer);

    // draw player rectangle

    const SDL_Rect playerRectangle = {MousePosition.X, MousePosition.Y, cPlayerSize.X, cPlayerSize.Y};

    SDL_SetRenderDrawColor(SDLGlobals.Renderer, 0, 255, 0, 255);
    SDL_RenderDrawRect(SDLGlobals.Renderer, &playerRectangle);

    // Drawing all these little boxes is actually rather slow, I would not draw everything one pixel at a time,
    // remember you can draw a single texture but have hundreds of little invisible bounding boxes for collision.
    if(drawBoundingBoxes)
    {
        DrawBoundingBoxes();
    }

    SDL_RenderPresent(SDLGlobals.Renderer);




}

void FrameDelay(unsigned int targetTicks)
{
    // Block at 60 fps

    // ticks is in ms
    const unsigned int ticks = SDL_GetTicks();

    if (targetTicks < ticks) 
    {
        return;
    }

    if (targetTicks > ticks + cFrameDuration_ms) 
    {
        SDL_Delay(cFrameDuration_ms);
    } 
    else 
    {
        SDL_Delay(targetTicks - ticks);
    }
}

int main()
{
    // initialization
    SDLGlobals = InitSDL(cScreenResolution);

    GenerateCollisionReactangles();

    printf("Hit B to show bounding boxes.\n");
    printf("The screen background will be red if the player bounding box (green) intersects a hitbox (red), and dark blue otherwise.\n");
    printf("Use the mouse to move the player bounding box around.\n");

    // main loop
    unsigned int targetTicks = SDL_GetTicks() + cFrameDuration_ms;
    while(1)
    {


        const Input input = HandleInput();

        if(input.Quit)
        {
            break;
        }

        if(input.ToggleBoundingBoxRender)
        {
            if(m_DrawBoundingBoxes)
            {
                m_DrawBoundingBoxes = 0;
            }
            else
            {
                m_DrawBoundingBoxes = 1;
            }
        }

        IntVec2_t playerPosition = MousePosition;
        const int didCollide = HasCollided(playerPosition);

        Render(didCollide, m_DrawBoundingBoxes);
        FrameDelay(targetTicks);
        targetTicks = SDL_GetTicks() + cFrameDuration_ms;
    }

    return 0;
}