#include "stdafx.h"
#include "math.h"

void lerp(
    vr::HmdVector3_t a,
    vr::HmdVector3_t b,
    float weight,
    vr::HmdVector3_t & outResult
) {
    outResult.v[0] = lerp(a.v[0], b.v[0], weight);
    outResult.v[1] = lerp(a.v[1], b.v[1], weight);
    outResult.v[2] = lerp(a.v[2], b.v[2], weight);
}

float lerp(float a, float b, float weight) {
    return a * (1.0f - weight) + b * weight;
}

float rad2deg(float rad) {
    return rad / M_PI * 180.0f;
}
