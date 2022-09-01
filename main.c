// Standard
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_gpu.h>

#include <stdio.h>

#define MAX_ANIM_LEN 40
#define MAX_ANIM_COUNT 20
#define FRAME_DUR 0.01f

typedef struct _pair Pair;
typedef struct _animation Animation;
typedef struct timespec Timespec;

typedef enum _animation_key AnimationKey;

enum _animation_key
{
    IDLE1,
    CROUCH,
    RUN,
    JUMP,
    MID,
    FALL,
    SLIDE,
    GRAB,
    CLIMB,
    IDLE2,
    ATTACK1,
    ATTACK2,
    ATTACK3,
    HURT,
    DIE,
    JUMP2,
    ANIMATION_MAX
};

struct _pair
{
    int a;
    int b;
};

struct _animation
{
    AnimationKey name;
    int length;
    Pair frames[MAX_ANIM_LEN];
};

Animation animations[MAX_ANIM_COUNT] = {
    {IDLE1, 4, { {0, 0}, {0, 1}, {0, 2}, {0, 3} }},
    {CROUCH, 4, { {0, 4}, {0, 5}, {0, 6}, {1, 0} }},
    {RUN, 6, { {1, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5}, {1, 6} }},
    {JUMP, 4, { {2, 0}, {2, 1}, {2, 2}, {2, 3} }},
    {MID, 4, { {2, 4}, {2, 5}, {2, 6}, {3, 0} }},
    {FALL, 2, { {3, 1}, {3, 2} }},
    {SLIDE, 5, { {3, 3}, {3, 4}, {3, 5}, {3, 6}, {4, 0} }},
    {GRAB, 4, { {4, 1}, {4, 2}, {4, 3}, {4, 4}}},
    {CLIMB, 5, { {4, 5}, {4, 6}, {5, 0}, {5, 1}, {5, 2} }},
    {IDLE2, 4, { {5, 3}, {5, 4}, {5, 5}, {5, 6} }},
    {ATTACK1, 5, { {6, 0}, {6, 1}, {6, 2}, {6, 3}, {6, 4} }},
    {ATTACK2, 6, { {6, 5}, {6, 6}, {7, 0}, {7, 1}, {7, 2}, {7, 3} }},
    {ATTACK3, 6, { {7, 4}, {7, 5}, {7, 6}, {8, 0}, {8, 1}, {8, 2} }},
    {HURT, 3, { {8, 3}, {8, 4}, {8, 5} }},
    {DIE, 6, { {8, 6}, {9, 0}, {9, 1}, {9, 2}, {9, 3}, {9, 4}, {9, 5} }},
    {JUMP2, 4, { {9, 6}, {10, 0}, {10, 1} }}
};

const int rows = 11, cols = 7;
const int width = 50, height = 37;

GPU_Target *windowPtr = NULL;
GPU_Image *hero = NULL;

double anim_speed = 24;
bool exe_done = false;
Pair current_pair;

static void init_sdl(void);
static void take_input(Animation *current, int *index);
static void prepare_sprites(GPU_Rect *sprites);
static void capture_unused_cmd_args(int argc, char *argv[]);
static void take_time(Timespec *ts);
static void try_next_frame(Animation *anim, double *time_buffer, int *index);
static void draw_frame(GPU_Rect *rectPtr, Animation *animPtr, int f);
static double time_spec_seconds(Timespec* ts);

int main(int argc, char *argv[])
{ 
    init_sdl();

    GPU_Rect sprites[rows * cols];
    Animation current = animations[IDLE1];
    int index = 0;
    double time_buffer = 0, time_elapsed = 0, elapsed_nano;
    Timespec tstart = {0,0}, tend = {0,0}, tadjust = {0,0};

    prepare_sprites(sprites);

    while(!exe_done) 
    {
        elapsed_nano = 0; 

        take_time(&tstart);
        take_input(&current, &index);
        draw_frame(sprites, &current, index);

        time_buffer += time_elapsed;

        try_next_frame(&current, &time_buffer, &index);
       
        SDL_Delay(50);

        take_time(&tend);
        elapsed_nano = time_spec_seconds(&tend) - time_spec_seconds(&tstart);

        SDL_Delay(50);

        take_time(&tadjust);
        time_elapsed = time_spec_seconds(&tadjust) - time_spec_seconds(&tstart);

    }

    capture_unused_cmd_args(argc, argv);

    GPU_FreeImage(hero);
    GPU_FreeTarget(windowPtr);
    GPU_Quit();
    SDL_Quit();

    return 0;
}

static void init_sdl(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("FATAL ERROR: SDL_INIT_VIDEO failed!\n");
    }
    windowPtr = GPU_InitRenderer(GPU_RENDERER_OPENGL_3, 200, 200, GPU_DEFAULT_INIT_FLAGS);
    if (windowPtr == NULL)
    {
        printf("FATAL ERROR: GPU_InitRenderer failed!\n");
    }
    hero = GPU_LoadImage("gfx/adventurer-sheet.png");
    if (hero == NULL)
    {
        printf("GPU_LoadImage failed!\n");
    }
}

static void prepare_sprites(GPU_Rect *sprites)
{
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            sprites[(i * cols) + j] = (GPU_Rect){ (float)(j * width), (float)(i * height), (float)width, (float)height };
        }
    }
}

static void take_time(Timespec *ts)
{
    clock_gettime(CLOCK_REALTIME, ts);
}

static void take_input(Animation *current, int *index)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            exe_done = true;
        }
        else if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_ESCAPE)
                exe_done = true;

            if (event.key.keysym.scancode == SDL_SCANCODE_Q)
            {
                *current = animations[IDLE1];
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_W)
            {
                *current = animations[CROUCH];
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_E)
            {
                *current = animations[RUN];
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_R)
            {
                *current = animations[JUMP];
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_T)
            {
                *current = animations[MID];
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_Y)
            {
                *current = animations[FALL];
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_U)
            {
                *current = animations[SLIDE];
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_I)
            {
                *current = animations[GRAB];
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_O)
            {
                *current = animations[CLIMB];
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_P)
            {
                *current = animations[IDLE2];
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_A)
            {
                *current = animations[ATTACK1];
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_S)
            {
                *current = animations[ATTACK2];
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_D)
            {
                *current = animations[ATTACK3];
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_F)
            {
                *current = animations[HURT];
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_G)
            {
                *current = animations[DIE];
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_H)
            {
                *current = animations[JUMP];
            }
            *index = 0;
        }
    }
}

static void draw_frame(GPU_Rect *rectPtr, Animation *animPtr, int f)
{
    GPU_Clear(windowPtr);

    current_pair = animPtr->frames[f];
    int position = (int)((current_pair.a * cols) + current_pair.b);
    GPU_BlitTransformX(hero, &rectPtr[position], windowPtr,
                       75, 75, 0, 0, 0, 1, 1);

    GPU_Flip(windowPtr);
}

static void try_next_frame(Animation *anim, double *time_buffer, int *index)
{
    if (*time_buffer > (double)(1/anim_speed))
    {
        *time_buffer = 0;
        (*index)++;

        if (*index >= anim->length)
        {
            *index = 0;
        }
    }
}
static void capture_unused_cmd_args(int argc, char *argv[])
{
    char *capture[argc];
        for (int i = 0; i < argc; i++)
        {
            int c = 0;
            while(*(&argv[i] + c)){
                *(&capture[i] + c) = *(&argv[i] + c);
            }
        }
}

static double time_spec_seconds(Timespec* ts) 
{
    return (double) ts->tv_sec + (double) ts->tv_nsec * 1.0e-9;
}