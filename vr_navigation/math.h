#pragma once

#include <openvr.h>

#define _USE_MATH_DEFINES
#include <math.h>

float lerp(float a, float b, float weight);

void lerp(
	vr::HmdVector3_t a,
	vr::HmdVector3_t b,
	float weight,
	vr::HmdVector3_t & outResult
);

float rad2deg(float rad);
