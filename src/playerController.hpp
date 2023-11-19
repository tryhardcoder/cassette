#pragma once

#include "engine.hpp"
#include <stdio.h>


// constants
V2f gravity = { 0, -30 };
float maxVelocity = 6;
float velocitySnap = 0.15; // percent of target vel to set to every frame

float rollSpeed = 10;
float rollSnap = 0.25;

V2f punchOffset = { 0.8, 0.5 };

Animation run = { };
Animation idle = { };
Animation punch = { };
Animation roll = { };
Animation hurt = { };


void s_playerTick(Entity* e) {
    PlayerState* p = &e->playerState;

    if(p->ctrlState == ps_controllable || p->ctrlState == ps_punching || p->ctrlState == ps_hurt) {
        float velTarget = engGlobs.inputs[INPUT_MOVEX].val * maxVelocity;
        e->velocity.x = lerp(e->velocity.x, velTarget, velocitySnap);

        if(p->jumpBuffered && p->grounded) {
            e->playerState.jumpBuffered = false;
            e->velocity.y = 10;
        }
        p->grounded = false;
    }
    else if(p->ctrlState == ps_rolling) {
        e->velocity.x = lerp(e->velocity.x, rollSpeed * (p->facingRight?1:-1), rollSnap);
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
    if(engGlobs.paused) { return; }

    PlayerState* p = &e->playerState;

    if(engGlobs.time - p->jumpBufferTime > 0.1) {
        p->jumpBuffered = false;
    }

    Input* i = &engGlobs.inputs[INPUT_JUMP];
    if(i->val) {
        p->jumpBuffered = i? true : false;
        p->jumpBufferTime = engGlobs.time;
    }

    if(p->ctrlState == ps_controllable || p->ctrlState == ps_hurt) {
        if(engGlobs.inputs[INPUT_MOVEX].val < 0) {
            p->facingRight = false;
        }
        if(engGlobs.inputs[INPUT_MOVEX].val > 0) {
            p->facingRight = true;
        }
        if(p->facingRight) {
            e->scale.x = fabsf(e->scale.x);
        } else {
            e->scale.x = -fabsf(e->scale.x);
        }
    }

    {
        Animation* prevAnim = e->animation;

        if(p->ctrlState == ps_controllable) {
            if(engGlobs.inputs[INPUT_PUNCH].val) {
                e->animation = &punch;
                p->ctrlState = ps_punching;
            } else if(engGlobs.inputs[INPUT_ROLL].val) {
                e->animation = &roll;
                p->ctrlState = ps_rolling;
            } else if(engGlobs.inputs[INPUT_MOVEX].val != 0) {
                e->animation = &run;
                p->ctrlState = ps_controllable;
            } else {
                if(e->animFrame == e->animation->frameCount-1 || e->animation == &run) {
                    e->animation = &idle;
                    p->ctrlState = ps_controllable;
                }
            }
        }

        if(e->animation != prevAnim) {
            e->animFrame = 0;
        };


        // early control returns
        if(e->animation == &roll && e->animFrame >= roll.frameCount - 12) {
            p->ctrlState = ps_controllable;
        }
        if(e->animation == &punch && e->animFrame >= punch.frameCount - 6) {
            p->ctrlState = ps_controllable;
        }
        if(e->animation == &hurt && e->animFrame >= hurt.frameCount - 1) {
            p->ctrlState = ps_controllable;
        }

        // punch box state management
        if(e->animation == &punch && (e->animFrame < punch.frameCount - 7) && (e->animFrame > 10)) {
            V2f dir = { (p->facingRight? 1.0f:-1.0f), 1 };
            p->punchBox->position = e->position + punchOffset * dir;
            p->punchBox->mask = phl_hurtbox;
            p->punchBox->hitboxState.direction.x = dir.x;
        }
        else {
            p->punchBox->mask = 0;
        }
    }
}

void s_playerCollide(p_Manifold m, Entity* e, Entity* other) {
    if(other->layer & phl_hitbox) {
        if(other == e->playerState.punchBox) { return; }
        if(e->animation == &hurt) { return; }
        if(e->playerState.ctrlState == ps_rolling) { return; }
        e->animation = &hurt;
        e->playerState.ctrlState = ps_hurt;
        e->animFrame = 0;
    }
    else {
        if(m.normal.y != 0) {
            e->velocity.y = 0;
        } else if(m.normal.x != 0) {
            e->velocity.x = 0;
        }
        e->position += m.depth * m.normal * -1;

        if(m.normal.y == -1) {
            e->playerState.grounded = true;
        }
    }
}
