#pragma once
#include "engine.hpp"
#include <stdio.h>

float hitCooldown = 0.5;

float hitTime = 0;

void s_dummyHit(p_Manifold m, Entity* e, Entity* other) {
    if(!(other->layer & phl_hitbox)) { return; }

    if(engGlobs.time - hitTime > hitCooldown) {
        e->position += v2fNormalized(other->hitboxState.direction) * 0.2;
        hitTime = engGlobs.time;
    }
}
