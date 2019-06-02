#pragma once

#include <openvr.h>

#include "log.h"
#include "async.h"

struct MyControllerState {
	vr::TrackedDeviceIndex_t index;
	bool dragging = false;
};

class MyVRStuff
{
public:
	MyVRStuff();
	~MyVRStuff();

	void start();
	/// Called automatically on destroy
	void stop();

private:
	Timer timer;

	bool vrInitialized = false;
	vr::IVRSystem* vrSystem = nullptr;

	const vr::TrackingUniverseOrigin universe = vr::TrackingUniverseStanding;

	MyControllerState leftControllerState  { 0 };
	MyControllerState rightControllerState { 1 };

	void onTick();
	void printDebugInfo();
	void checkButtonsChange();
	void updatePosition();

	bool getDraggedPoint(const double* & outDragPoint, double & outDragYaw);
	bool getControllerPosition(vr::TrackedDeviceIndex_t controllerIndex) const;
	bool isDragButtonHeld(const vr::VRControllerState_t & controllerState) const;
	void setPositionRotation();
};
