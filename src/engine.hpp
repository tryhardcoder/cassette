#pragma once
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "base/typedefs.h"
#include "base/geometry.h"
#include "base/allocators.h"

struct Entity;


struct UniBlock {
    U32 textureId = 0;
    U32 fontTextureId = 0;
    V2f srcStart = V2f();
    V2f srcEnd = V2f(1, 1);
    V2f dstStart = V2f();
    V2f dstEnd = V2f(1, 1);
    V2f clipStart = V2f();
    V2f clipEnd = V2f(1, 1);
    Mat4f model = Mat4f(1.0);
    V4f color = { 0, 0, 0, 1 };
    UniBlock* next = nullptr;
    float cornerRadius = 0;
    float borderSize = 0;
    V4f borderColor = { 0, 0, 0, 0};
};

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



struct Animation {
    U32 frameCount = 0;
    Texture* tex = nullptr;
};


enum PlayerCtrlState {
    ps_controllable,
    ps_rolling,
    ps_punching,
    ps_hurt,
};
struct PlayerState {
    bool jumpBuffered = false;
    float jumpBufferTime = 0;
    bool grounded = false;

    bool facingRight = true;
    PlayerCtrlState ctrlState = ps_controllable;

    Entity* punchBox = nullptr;
};

struct HitboxData {
    V2f direction = { 0,0 };
};

enum {
    phl_collision = (1),
    phl_hitbox = (1<<1),
    phl_hurtbox = (1<<2),
};


enum EntityFlag {
    entityFlag_none =       0,
    entityFlag_render =     (1 << 0),
    entityFlag_collision =  (1 << 1),
    entityFlag_animation =  (1 << 2),
};

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

    PlayerState playerState;
    HitboxData hitboxState;
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
#define INPUT_PAUSE_TOGGLE 5

struct eng_Globs {
    BumpAlloc frameArena;
    BumpAlloc levelArena;
    Entity* firstEntity = nullptr;
    U32 entityCount = 0;

    Input inputs[6] = {
        { 0, 0, GLFW_KEY_D, GLFW_KEY_A },
        { 0, 0, GLFW_KEY_SPACE, 0 },
        { 0, 0, GLFW_KEY_J, 0 },
        { 0, 0, GLFW_KEY_K, 0 },
        { 0, 0, GLFW_KEY_TAB, 0 },
        { 0, 0, GLFW_KEY_ESCAPE, 0 },
    };

    float dt = 0;
    float time = 0;
};

extern eng_Globs engGlobs;

void initGlobs();
UniBlock* registerCall(UniBlock** listHead, BumpAlloc* arena);
Texture* makeTexture(const char* path, int bpp, BumpAlloc* arena);
Texture* makeTexture(U8* data, const int width, const int height, int bpp, BumpAlloc* arena);
bool p_AABB_intersect(V2f aHs, V2f bHs, V2f aPos, V2f bPos, p_Manifold* out);

#ifdef ENG_IMPL
#include "GLAD/gl.h"
#include "stb_image/stb_image.h"

#include <math.h>

eng_Globs engGlobs;

void initGlobs() {
    bump_allocate(&engGlobs.levelArena, 10000000);
    bump_allocate(&engGlobs.frameArena, 10000000);
}

UniBlock* registerCall(UniBlock** listHead, BumpAlloc* arena) {
    UniBlock* b = BUMP_PUSH_NEW(arena, UniBlock);
    b->next = *listHead;
    *listHead = b;
    return b;
}

Texture* makeTexture(U8* data, const int width, const int height, int bpp, BumpAlloc* arena) {
    Texture* t = BUMP_PUSH_NEW(arena, Texture);
    t->width = width;
    t->height = height;

    glGenTextures(1, &t->id);
    glBindTexture(GL_TEXTURE_2D, t->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // NOTE: for pixely looks again, use -> GL_NEAREST as the mag/min filter

    if(bpp == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, t->width, t->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    } else if(bpp == 1) {
        // They lied??????????????????????????
        // glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction, who knows what thisl do
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, t->width, t->height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    }
    else { assert(false); }
    return t;
}

Texture* makeTexture(const char* path, int bpp, BumpAlloc* arena) {
    // CLEANUP: image data is performing an allocaiton that I don't like
    stbi_set_flip_vertically_on_load(1);
    S32 loadedBpp, w, h;
    U8* data = stbi_load(path, &w, &h, &loadedBpp, bpp);
    ASSERT(data);
    return makeTexture(data, w, h, bpp, arena);
}


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


#endif