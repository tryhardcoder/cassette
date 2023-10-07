#pragma once
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "base/typedefs.h"
#include "base/geometry.h"
#include "base/allocators.h"
#include <math.h>


typedef struct {
    U32 textureId = 0;
    V2f srcStart = V2f();
    V2f srcEnd = V2f(1, 1);
    Mat4f model = Mat4f(1.0);
    V4f color = { 0, 0, 0, 1 };
} UniBlock;

typedef struct {
    U32 id = 0;
    U32 width = 0;
    U32 height = 0;
} Texture;

#define PHYSICS_DT (1.0/60.0)

typedef struct {
    V2f normal;
    float depth;
} p_Manifold;

bool p_AABB_intersect(V2f aHs, V2f bHs, V2f aPos, V2f bPos, p_Manifold* out) {
    V2f n = bPos - aPos;
    float yOverlap = aHs.y + bHs.y - fabs(n.y);
    if (yOverlap > 0) {
        float xOverlap = aHs.x + bHs.x - fabs(n.x);
        if(xOverlap > 0) {
            if(xOverlap < yOverlap) {
                if (n.x < 0) {
                    out->normal = V2f(-1, 0);
                } else {
                    out->normal = V2f(1, 0);
                }
                out->depth = xOverlap;
            } else {
                if (n.y < 0) {
                    out->normal = V2f(0, -1);
                } else {
                    out->normal = V2f(0, 1);
                }
                out->depth = yOverlap;
            }
            return true;
        }
    }
    return false;
}


struct Animation {
    U32 frameCount = 0;
    Texture* tex = nullptr;
};


enum EntityFlag {
    entityFlag_none =       0,
    entityFlag_render =     (1 << 0),
    entityFlag_tickFunc =   (1 << 1),
    entityFlag_frameFunc =  (1 << 2),
    entityFlag_collision =  (1 << 3),
    entityFlag_animation =  (1 << 4),
};

typedef struct Entity Entity;
typedef void (*EntityUpdateFunc)(Entity* e);
typedef void (*p_CollideFunc)(p_Manifold m, Entity* e, Entity* other);
struct Entity {
    V2f textureStart = { 0, 0 };
    V2f textureEnd = { 1, 1 };
    Texture* texture = nullptr;
    V2f position = V2f();
    V2f velocity = V2f();

    Animation* animation = nullptr;
    U32 animFrame = 0;
    float animAcc = 0;
    float animFPS = (1/30.0);

    V2f colliderHalfSize = V2f();
    U32 layer = 0;
    U32 mask = 0;

    float zIndex = 0;
    V2f scale = V2f(1, 1);

    EntityUpdateFunc tickFunc = nullptr;
    EntityUpdateFunc frameFunc = nullptr;
    p_CollideFunc collideFunc = nullptr;

    U32 flags = 0;
    Entity* next = nullptr;
};


typedef struct {
    float val = 0;
    float prev = 0;
    S32 plusKey = 0;
    S32 minKey = 0;
} Input;


#define INPUT_MOVEX 0
#define INPUT_JUMP 1
#define INPUT_PUNCH 2
#define INPUT_ROLL 3
#define INPUT_DEBUG_DRAW_TOGGLE 4

struct Globs {
    BumpAlloc frameArena;
    BumpAlloc levelArena;
    Entity* firstEntity = nullptr;
    U32 entityCount = 0;

    Input inputs[5] = {
        { 0, 0, GLFW_KEY_D, GLFW_KEY_A },
        { 0, 0, GLFW_KEY_SPACE, 0 },
        { 0, 0, GLFW_KEY_J, 0 },
        { 0, 0, GLFW_KEY_K, 0 },
        { 0, 0, GLFW_KEY_TAB, 0 },
    };

    float dt = 0;
    float time = 0;
} globs;

void initGlobs() {
    bump_allocate(&globs.levelArena, 10000000);
    bump_allocate(&globs.frameArena, 10000000);
}