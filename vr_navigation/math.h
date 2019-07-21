#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

#include "shared/Matrices.h"

float lerp(float a, float b, float weight);

void lerp(
	Vector3 a,
	Vector3 b,
	float weight,
	Vector3 & outResult
);

float rad2deg(float rad);
