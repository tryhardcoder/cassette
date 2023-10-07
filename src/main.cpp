
#include <stdio.h>
#include <math.h>

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "stb_image/stb_image.h"
#include "stb_truetype/stb_truetype.h"
#include "GLAD/gl.h"

#include "base/utils.h"
#include "base/geometry.h"

#include "engine.hpp"
#include "playerController.hpp"

Texture* makeTexture(const char* path, BumpAlloc* arena) {
    // CLEANUP: image data is performing an allocaiton that I don't like
    stbi_set_flip_vertically_on_load(1);
    S32 bpp, w, h;
    U8* data = stbi_load(path, &w, &h, &bpp, 4);
    ASSERT(data);

    Texture* t = BUMP_PUSH_NEW(arena, Texture);
    t->width = w;
    t->height = h;

    glGenTextures(1, &t->id);
    glBindTexture(GL_TEXTURE_2D, t->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // NOTE: for pixely looks again, use -> GL_NEAREST as the mag/min filter

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, t->width, t->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return t;
}

// asserts of failure
U32 registerShader(const char* vPath, const char* fPath) {
    U32 shaderId = 0;
    int err = 0;

    U64 vertSize;
    U8* vertSrc = loadFileToBuffer(vPath, true, &vertSize, &globs.frameArena);
    assert(vertSrc);

    U64 fragSize;
    U8* fragSrc = loadFileToBuffer(fPath, true, &fragSize, &globs.frameArena);
    assert(fragSrc);

    //compile src into shader code
    U32 v = glCreateShader(GL_VERTEX_SHADER);
    int smallVertSize = (int)vertSize;
    glShaderSource(v, 1, (const char* const*)&vertSrc, &smallVertSize);
    glCompileShader(v);

    U32 f = glCreateShader(GL_FRAGMENT_SHADER);
    int smallFragSize = (int)fragSize; // CLEANUP: these could overflow
    glShaderSource(f, 1, (const char* const*)&fragSrc, &smallFragSize);
    glCompileShader(f);

    bool compFailed = false;
    char* logBuffer = BUMP_PUSH_ARR(&globs.frameArena, 512, char);

    glGetShaderiv(v, GL_COMPILE_STATUS, &err);
    if(!err) {
        glGetShaderInfoLog(v, 512, NULL, logBuffer);
        printf("Compiling shader \"%s\" failied: %s", "2d.vert", logBuffer);
        compFailed = true;
    };

    glGetShaderiv(f, GL_COMPILE_STATUS, &err);
    if(!err) {
        glGetShaderInfoLog(f, 512, NULL, logBuffer);
        printf("Compiling shader \"%s\" failied: %s", "2f.frag", logBuffer);
        compFailed = true;
    };


    if(!compFailed) {
        shaderId = glCreateProgram();
        glAttachShader(shaderId, v);
        glAttachShader(shaderId, f);

        glLinkProgram(shaderId);
        glValidateProgram(shaderId);
    }
    else {
        assert(false);
    }

    glDeleteShader(v);
    glDeleteShader(f);

    return shaderId;
}

Entity* registerEntity() {
    Entity* e = BUMP_PUSH_NEW(&globs.levelArena, Entity);
    SLL_PUSH_FRONT(e, globs.firstEntity);
    globs.entityCount++;
    return e;
}

int main() {
    initGlobs();

    // GLFW INIT AND WINDOW CREATION ===============================================================================
    GLFWwindow* window = nullptr;
    {
        assert(glfwInit());
        glfwSetErrorCallback([](int error, const char* description) { printf("[GLFW] %s\n", description); });

        glfwWindowHint(GLFW_MAXIMIZED, true);
        glfwWindowHint(GLFW_RESIZABLE, true);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_SAMPLES, 4);

        window = glfwCreateWindow(512, 512, "GAME OF ALL TIME", NULL, NULL);
        ASSERT(window);

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
    }

    // OPENGL/GLAD INIT
    {
        gladLoadGL(glfwGetProcAddress);
        glLoadIdentity();

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthFunc(GL_LESS | GL_EQUAL);

        // glLineWidth(3);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_MULTISAMPLE);

        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(
        [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
            if(type == GL_DEBUG_TYPE_OTHER) { return; } // hides messages talking about buffer memory source which were getting spammed
            printf("[GL] %i, %s\n", type, message);
        }, 0);
    }

    float cameraHeight = 10;
    V2f cameraPos = { 0, 3.9 };
    float cameraZ = 10;

    Texture* invalidTexture = makeTexture("res/textures/noTexture.png", &globs.levelArena);

    // loading player
    {
        Entity* e = registerEntity();

        idle.frameCount = 25;
        idle.tex = makeTexture("res/textures/idle.png", &globs.levelArena);
        run.frameCount = 20;
        run.tex = makeTexture("res/textures/run.png", &globs.levelArena);
        punch.frameCount = 24;
        punch.tex = makeTexture("res/textures/punch.png", &globs.levelArena);
        roll.frameCount = 29;
        roll.tex = makeTexture("res/textures/roll.png", &globs.levelArena);

        e->animation = &idle;
        e->flags |= entityFlag_animation;

        e->scale = { 0.8, 0.8 };
        e->flags |= entityFlag_render;

        e->tickFunc = s_playerTick;
        e->flags |= entityFlag_tickFunc;
        e->frameFunc = s_playerFrame;
        e->flags |= entityFlag_frameFunc;

        e->collideFunc = s_playerCollide;
        e->colliderHalfSize = { 0.4, 0.8 };
        e->layer = 1;
        e->mask = 1;
        e->flags |= entityFlag_collision;
    }

    {
        Entity* e = registerEntity();
        e->flags |= entityFlag_collision;
        e->layer = (1<<1);
        e->mask = (1<<1);
        e->colliderHalfSize = { 0.5, 0.25 };
    }

    {
        Entity* e = registerEntity();
        e->texture = makeTexture("res/textures/background.png", &globs.levelArena);
        e->scale = { cameraHeight/2, cameraHeight/2 };
        e->flags |= entityFlag_render;
        e->zIndex = -1;
        e->position = cameraPos;
    }

    // dummy
   {
        Entity* e = registerEntity();
        e->texture = makeTexture("res/textures/dummy.png", &globs.levelArena);
        e->scale = {1, 1};
        e->flags |= entityFlag_render;

        e->mask = (1<<1);
        e->layer = (1<<1);
        e->flags |= entityFlag_collision;
        e->colliderHalfSize = { 0.3, 1 };

        e->position.x = 4;
    }

    // ground
    {
        float width = 40;
        Entity* e = registerEntity();
        e->position = V2f(0, -2);

        e->colliderHalfSize = { width/2, 1 };
        /*
        float aspect = e->texture->width / (float)e->texture->height;
        e->textureEnd = { (e->scale.x / aspect), 1 };
        */
        e->layer = 1;
        e->mask = 1;
        e->flags |= entityFlag_collision;
    }
    {
        Entity* e = registerEntity();
        e->texture = makeTexture("res/textures/brick.png", &globs.levelArena);
        e->flags |= entityFlag_render;
        e->position = V2f(-10, 0);
        float height = 1;
        e->scale = { 1, height/2 };

        e->colliderHalfSize = { 1, height/2 };
        e->layer = 1;
        e->mask = 1;
        e->flags |= entityFlag_collision;
    }


    U32 ib = 0;
    {
        U32 data[] = { 0, 1, 2, 2, 3, 0 };
        U32 dataSize = sizeof(data);
        glGenBuffers(1, &ib);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, dataSize, data, GL_STATIC_DRAW);
    }

    U32 va = 0;
    {
        glGenVertexArrays(1, &va);
        glBindVertexArray(va);

        U32 vb = 0;
        glGenBuffers(1, &vb);
        glBindBuffer(GL_ARRAY_BUFFER, vb);

        float data[] = { -1, -1, -1, 1, 1, 1, 1, -1 };
        U32 dataSize = sizeof(data);
        glBufferData(GL_ARRAY_BUFFER, dataSize, data, GL_STATIC_DRAW);

        U32 vertSize = 2 * sizeof(F32);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, vertSize, (void*)(0));
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    U32 sceneShader = registerShader("res/shaders/2d.vert", "res/shaders/2d.frag");
    U32 solidShader = registerShader("res/shaders/solid.vert", "res/shaders/solid.frag");

    BumpAlloc renderCallArena;
    bump_allocate(&renderCallArena, sizeof(UniBlock) * 256);
    BumpAlloc debugCallArena;
    bump_allocate(&debugCallArena, sizeof(UniBlock) * 256);


    bool debugDrawEnabled = true;

    F64 acc = 0;
    F64 prevTime = glfwGetTime();
    while(!glfwWindowShouldClose(window)) {

        {
            Input* inp = &globs.inputs[INPUT_DEBUG_DRAW_TOGGLE];
            if(inp->val && !inp->prev){
                debugDrawEnabled = !debugDrawEnabled;
            }
        }

        globs.time = glfwGetTime();
        globs.dt = globs.time - prevTime;
        prevTime = globs.time;

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);


        // UPDATE INPUTS
        {
            for(int i = 0; i < sizeof(globs.inputs) / sizeof(Input); i++) {
                Input* input = &globs.inputs[i];
                input->prev = input->val;

                input->val = 0;
                if(input->plusKey) {
                    input->val += glfwGetKey(window, input->plusKey);
                }
                if(input->minKey) {
                    input->val -= glfwGetKey(window, input->minKey);
                }
            }
        }

        // TICK UPDATES
        {
            acc += globs.dt;
            if(acc > 0.2) { acc = 0.2; }
            while(acc > PHYSICS_DT) {
                acc -= PHYSICS_DT;


                Entity** physicsList = BUMP_PUSH_ARR(&globs.frameArena, globs.entityCount*globs.entityCount, Entity*);
                U32 physicsCount = 0;

                Entity* e = globs.firstEntity;
                while(e) {
                    if(e->flags & entityFlag_collision) {
                        ARR_APPEND(physicsList, physicsCount, e);
                    }
                    e = e->next;
                }

                typedef struct {
                    Entity* a;
                    Entity* b;
                } Pair;
                Pair* pairs = BUMP_PUSH_ARR(&globs.frameArena, physicsCount*physicsCount, Pair);
                U32 pairCount = 0;

                for(int i = 0; i < physicsCount; i++) {
                    Entity* a = physicsList[i];
                    for(int j = i+1; j < physicsCount; j++) {
                        Entity* b = physicsList[j];
                        if(a->mask & b->layer || b->mask & a->layer) {
                            ARR_APPEND(pairs, pairCount, (Pair{a, b}));
                        }
                    }
                }

                for(int i = 0; i < pairCount; i++) {
                    Pair p = pairs[i];
                    p_Manifold m;
                    if(p_AABB_intersect(
                        p.a->colliderHalfSize, p.b->colliderHalfSize,
                        p.a->position, p.b->position,
                        &m)) {
                        if(p.a->collideFunc) {
                            p.a->collideFunc(m, p.a, p.b);
                        }
                        if(p.b->collideFunc) {
                            m.normal *= -1;
                            p.b->collideFunc(m, p.b, p.a);
                        }
                    }
                }

                // TICKS
                e = globs.firstEntity;
                while(e) {
                    if(e->flags & entityFlag_tickFunc) {
                        e->tickFunc(e);
                    }
                    e = e->next;
                }

            } // end acc loop
        } // end section

        // FRAME UPDATES AND CALL CREATION
        bump_clear(&renderCallArena);
        bump_clear(&debugCallArena);

        Entity* e = globs.firstEntity;
        while(e) {
            if(e->flags & entityFlag_animation) {
                e->animAcc += globs.dt;

                if(e->animAcc > e->animFPS) {
                    e->animAcc -= e->animFPS;
                    e->animFrame++;
                    if(e->animFrame >= e->animation->frameCount) {
                        e->animFrame = e->animFrame % e->animation->frameCount;
                    }
                }

                float offset = 1.0 / e->animation->frameCount;
                e->texture = e->animation->tex;
                e->textureStart = { e->animFrame * offset, 0 };
                e->textureEnd = { (e->animFrame+1) * offset, 1 };
            }
            if(e->flags & entityFlag_frameFunc) {
                e->frameFunc(e);
            }
            if(e->flags & entityFlag_render) {
                UniBlock* b = BUMP_PUSH_NEW(&renderCallArena, UniBlock);
                float aspect = 1;
                if(e->texture) {
                    b->textureId = e->texture->id;
                    b->srcStart = e->textureStart;
                    b->srcEnd = e->textureEnd;
                    float texAspect = (float)e->texture->width / (float)e->texture->height;
                    V2f subTexSize = e->textureEnd - e->textureStart;
                    aspect = (subTexSize.x / subTexSize.y) * texAspect;
                    aspect = fabsf(aspect);
                } else {
                    b->textureId = invalidTexture->id;
                }
                b->model = matrixTransform(
                    e->position.x, e->position.y,
                    e->zIndex,
                    0,
                    e->scale.x * aspect, e->scale.y);
            }
            if(debugDrawEnabled) {
                if(e->flags & entityFlag_collision) {
                    UniBlock* b = BUMP_PUSH_NEW(&debugCallArena, UniBlock);
                    b->color = V4f(0, 1, 0, 1);
                    b->model = matrixTransform(
                        e->position.x, e->position.y,
                        0.00001,
                        0,
                        e->colliderHalfSize.x, e->colliderHalfSize.y);
                }
            }
            e = e->next;
        }

        // DRAW RENDER CALLS
        {
            Mat4f view = Mat4f(1.0f);
            Mat4f temp = Mat4f(1.0f);
            matrixTranslation(cameraPos.x, cameraPos.y, cameraZ, view);
            matrixInverse(view, view);
            Mat4f proj;
            float aspect = (float)w / (float)h;
            float hh = cameraHeight / 2.0;
            matrixOrtho(-hh*aspect, hh*aspect, -hh, hh, 0.001, 10000, proj);
            Mat4f vp = view * proj;

            glViewport(0, 0, w, h);

            glClearColor(1, 1, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // scene objects
            {
                glUseProgram(sceneShader);
                glBindVertexArray(va);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
                int loc = glGetUniformLocation(sceneShader, "uVP");
                glUniformMatrix4fv(loc, 1, false, &vp[0]);

                int len = (UniBlock*)renderCallArena.end - (UniBlock*)renderCallArena.start;
                for(int i = 0; i < len; i++) {
                    UniBlock* b = &((UniBlock*)renderCallArena.start)[i];
                    loc = glGetUniformLocation(sceneShader, "uTexture");
                    glUniform1i(loc, 0);
                    glActiveTexture(GL_TEXTURE0 + 0);
                    glBindTexture(GL_TEXTURE_2D, b->textureId);
                    loc = glGetUniformLocation(sceneShader, "uModel");
                    glUniformMatrix4fv(loc, 1, false, &b->model[0]);
                    loc = glGetUniformLocation(sceneShader, "uSrcStart");
                    glUniform2f(loc, b->srcStart.x, b->srcStart.y);
                    loc = glGetUniformLocation(sceneShader, "uSrcEnd");
                    glUniform2f(loc, b->srcEnd.x, b->srcEnd.y);

                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
                }
            }


            // debug collider shapes
            {
                glUseProgram(solidShader);
                glBindVertexArray(va);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);

                int loc = glGetUniformLocation(solidShader, "uVP");
                glUniformMatrix4fv(loc, 1, false, &vp[0]);

                int len = (UniBlock*)debugCallArena.end - (UniBlock*)debugCallArena.start;
                for(int i = 0; i < len; i++) {
                    UniBlock* b = &((UniBlock*)debugCallArena.start)[i];

                    loc = glGetUniformLocation(solidShader, "uColor");
                    glUniform4f(loc, b->color.x, b->color.y, b->color.z, b->color.w);

                    loc = glGetUniformLocation(solidShader, "uModel");
                    glUniformMatrix4fv(loc, 1, false, &b->model[0]);
                    glDrawElements(GL_LINE_LOOP, 6, GL_UNSIGNED_INT, nullptr);
                }
            }
        }

        bump_clear(&globs.frameArena);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glfwDestroyWindow(window);
    glfwTerminate();

    // getchar();
}
