#include "stdafx.h"
#include "math.h"

void lerp(
    Vector3 a,
    Vector3 b,
    float weight,
    Vector3 & outResult
) {
    outResult[0] = lerp(a[0], b[0], weight);
    outResult[1] = lerp(a[1], b[1], weight);
    outResult[2] = lerp(a[2], b[2], weight);
}

float lerp(float a, float b, float weight) {
    return a * (1.0f - weight) + b * weight;
}

double rad2deg(double rad) {
    return rad / M_PI * 180.0f;
}
