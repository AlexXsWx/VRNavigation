#pragma once

#include <openvr.h>

#include "log.h"
#include "async.h"

class MyVRStuff
{
public:
	MyVRStuff();
	~MyVRStuff();
	void start();
private:
	bool vrInitialized = false;
	vr::IVRSystem* vrSystem = nullptr;

	Timer timer;

	void updatePosition();
};
