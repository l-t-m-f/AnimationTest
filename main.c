// Standard
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_gpu.h>

#include <stdlib.h>

#define MAX_ANIM_LEN 40
#define MAX_ANIM_COUNT 20
#define FRAME_DUR 0.01f

//#define DEBUG

typedef uint8_t u8;

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
    u8 a;
    u8 b;
};

struct _animation
{
    AnimationKey name;
    u8 length;
    Pair frames[MAX_ANIM_LEN];
};

const u8 rows = 11, cols = 7;
const u8 width = 50, height = 37;

Pair current_pair;

GPU_Target *windowPtr = NULL;
GPU_Image *hero = NULL;

static void capture_unused_cmd_args(int argc, char *argv[]);
static double time_spec_seconds(struct timespec* ts);
static void take_time(Timespec *ts);
static bool try_next_frame(Timespec *tend, Timespec *tstart);
static void draw_frame(GPU_Rect *rectPtr, Animation *animPtr, u8 f);

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

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    windowPtr = GPU_InitRenderer(GPU_RENDERER_OPENGL_3, 200, 200, GPU_DEFAULT_INIT_FLAGS);
    hero = GPU_LoadImage("gfx/adventurer-sheet.png");

    if (hero == NULL)
    {
        printf("GPU_LoadImage failed!");
    }

    GPU_Rect sprites[rows * cols];

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            sprites[(i * cols) + j] = (GPU_Rect){ (float)(j * width), (float)(i * height), (float)width, (float)height };
        }
    }
    
    Animation current = animations[IDLE1];
    u8 index = 0;

    Timespec tstart = {0,0}, tend = {0,0}, tadjust = {0,0};
    SDL_Event event;
    bool done = false;

    take_time(&tstart);
    take_time(&tend);

    while(!done) 
    {
        
        while(SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                done = true;
            }
            else if (event.type == SDL_KEYDOWN) 
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    done = true;

                if (event.key.keysym.scancode == SDL_SCANCODE_Q)
                {
                    current = animations[IDLE1];
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_W)
                {
                    current = animations[CROUCH];
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_E)
                {
                    current = animations[RUN];
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_R)
                {
                    current = animations[JUMP];
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_T)
                {
                    current = animations[MID];
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_Y)
                {
                    current = animations[FALL];
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_U)
                {
                    current = animations[SLIDE];
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_I)
                {
                    current = animations[GRAB];
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_O)
                {
                    current = animations[CLIMB];
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_P)
                {
                    current = animations[IDLE2];
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_A)
                {
                    current = animations[ATTACK1];
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_S)
                {
                    current = animations[ATTACK2];
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_D)
                {
                    current = animations[ATTACK3];
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_F)
                {
                    current = animations[HURT];
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_G)
                {
                    current = animations[DIE];
                }
                else if (event.key.keysym.scancode == SDL_SCANCODE_H)
                {
                    current = animations[JUMP];
                }
                index = 0;
            }
        }

        draw_frame(sprites, &current, index);

        if(try_next_frame(&tend, &tstart))
        {
            index++;
            take_time(&tstart);

            if (index >= current.length)
            {
                index = 0;
            }
        }

        take_time(&tend);

    }

    capture_unused_cmd_args(argc, argv);

    GPU_FreeImage(hero);
    GPU_FreeTarget(windowPtr);
    GPU_Quit();
    SDL_Quit();

    return 0;
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

static void take_time(Timespec *ts)
{
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, ts);
#ifdef DEBUG
    printf("%lf\n", time_spec_seconds(ts));
#endif
}

static bool try_next_frame(Timespec *tend, Timespec *tstart)
{
    double time_passed = time_spec_seconds(tend) - time_spec_seconds(tstart);

#ifdef DEBUG
    printf("%lf - %lf = %lf\n", time_spec_seconds(tend), time_spec_seconds(tstart), time_passed);
#endif

    if (time_passed > FRAME_DUR)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void draw_frame(GPU_Rect *rectPtr, Animation *animPtr, u8 f)
{
    GPU_Clear(windowPtr);

    current_pair = animPtr->frames[f];
    int position = (int)((current_pair.a * cols) + current_pair.b);
    GPU_BlitTransformX(hero, &rectPtr[position], windowPtr,
                       75, 75, 0, 0, 0, 1, 1);

#ifdef DEBUG
    printf("Showing frame %d of %d\n", f, animPtr->length);
    printf("located at %d, %d on the spritesheet\n", current_pair.a, current_pair.b);
#endif

    GPU_Flip(windowPtr);
}

static void limit_fps(void)
{

}