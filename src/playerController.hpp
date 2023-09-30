#pragma once

#include "engine.hpp"
#include <stdio.h>

V2f gravity = { 0, -30 };
float maxVelocity = 8;
float velocitySnap = 0.15; // percent of target vel to set to every frame

bool playerJumpBuffered = false;
float playerJumpBufferTime = 0;
bool grounded = false;

int playerFrame = 0;
float playerFrameAcc = 0;
bool playerFacingRight = true;

void s_playerTick(Entity* e) {
    float velTarget = globs.inputs[INPUT_MOVEX].val * maxVelocity;
    e->velocity.x = lerp(e->velocity.x, velTarget, velocitySnap);
    e->velocity += gravity * PHYSICS_DT;
    if(playerJumpBuffered && grounded) {
        playerJumpBuffered = false;
        e->velocity.y = 10;
    }
    grounded = false;

    if(e->position.y < -10) {
        e->position = {0, 0};
    }

    e->position += e->velocity * PHYSICS_DT;
    e->velocity.x *= 0.95;
}

void s_playerFrame(Entity* e) {
    if(globs.time - playerJumpBufferTime > 0.1) {
        playerJumpBuffered = false;
    }

    Input* i = &globs.inputs[INPUT_JUMP];
    if(i->val) {
        playerJumpBuffered = i;
        playerJumpBufferTime = globs.time;
    }

    if(globs.inputs[INPUT_MOVEX].val < 0) {
        playerFacingRight = false;
    }
    if(globs.inputs[INPUT_MOVEX].val > 0) {
        playerFacingRight = true;
    }

    playerFrameAcc += globs.dt;
    if(playerFrameAcc > (1/30.0)) {
        playerFrameAcc = 0;
        playerFrame++;
        if(playerFrame >= 20) {
            playerFrame = playerFrame % 20;
        }

        float offset = 1.0 / 20.0;
        e->textureStart = { (playerFrame + !playerFacingRight)*offset, 0 };
        e->textureEnd = { (playerFrame + playerFacingRight)*offset, 1 };
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
