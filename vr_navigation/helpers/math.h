#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

#include "../shared/Matrices.h"

template<typename T>
const T & clamp(const T & value, const T & min, const T & max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float lerp(float a, float b, float weight);

void lerp(
    Vector3 a,
    Vector3 b,
    float weight,
    Vector3 & outResult
);

float rad2deg(float rad);
