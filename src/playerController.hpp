#pragma once

#include "engine.hpp"
#include <stdio.h>

V2f gravity = { 0, -30 };
float maxVelocity = 8;
float velocitySnap = 0.15; // percent of target vel to set to every frame

float rollSpeed = 10;

bool playerJumpBuffered = false;
float playerJumpBufferTime = 0;
bool grounded = false;

bool playerFacingRight = true;

Animation run = { };
Animation idle = { };
Animation punch = { };
Animation roll = { };

void s_playerTick(Entity* e) {

    if(e->animation == &idle || e->animation == &run) {
        float velTarget = globs.inputs[INPUT_MOVEX].val * maxVelocity;
        e->velocity.x = lerp(e->velocity.x, velTarget, velocitySnap);

        if(playerJumpBuffered && grounded) {
            playerJumpBuffered = false;
            e->velocity.y = 10;
        }
        grounded = false;
    }
    else if(e->animation == &roll) {
        e->velocity.x = lerp(e->velocity.x, rollSpeed * (playerFacingRight?-1:1), velocitySnap);
    }

    if(e->position.y < -10) {
        e->position = {0, 0};
    }

    e->velocity += gravity * PHYSICS_DT;
    e->position += e->velocity * PHYSICS_DT;
    e->velocity.x *= 0.95;
}

#define FRAME_COUNT 25
void s_playerFrame(Entity* e) {
    if(globs.time - playerJumpBufferTime > 0.1) {
        playerJumpBuffered = false;
    }

    Input* i = &globs.inputs[INPUT_JUMP];
    if(i->val) {
        playerJumpBuffered = i;
        playerJumpBufferTime = globs.time;
    }

    if(e->animation != &roll) {
        if(globs.inputs[INPUT_MOVEX].val < 0) {
            playerFacingRight = false;
        }
        if(globs.inputs[INPUT_MOVEX].val > 0) {
            playerFacingRight = true;
        }
        if(playerFacingRight) {
            e->scale.x = fabsf(e->scale.x);
        } else {
            e->scale.x = -fabsf(e->scale.x);
        }
    }

    {
        Animation* prevAnim = e->animation;

        if(e->animation != &punch || (e->animation == &punch && e->animFrame == punch.frameCount -1)) {
            if(e->animation != &roll || (e->animation == &roll && e->animFrame == roll.frameCount -5)) {
                if(globs.inputs[INPUT_PUNCH].val) {
                    e->animation = &punch;
                } else if(globs.inputs[INPUT_ROLL].val) {
                    e->animation = &roll;
                } else if(globs.inputs[INPUT_MOVEX].val != 0) {
                    e->animation = &run;
                } else {
                    e->animation = &idle;
                }
            }
        }

        if(e->animation != prevAnim) {
            e->animFrame = 0;
        };
    }
}

void s_playerCollide(p_Manifold m, Entity* e, Entity* other) {
    if(m.normal.y != 0) {
        e->velocity.y = 0;
    } else if(m.normal.x != 0) {
        e->velocity.x = 0;
    }
    e->position += m.depth * m.normal * -1;

    if(m.normal.y == -1) {
        grounded = true;
    }
}
