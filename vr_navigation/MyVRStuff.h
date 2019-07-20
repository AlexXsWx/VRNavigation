#pragma once

#include <openvr.h>

#include "log.h"
#include "async.h"

//

struct MyControllerState {
	vr::TrackedDeviceIndex_t index;
	bool dragging = false;
};

//

class MyVRStuff {

	public:

		MyVRStuff();
		~MyVRStuff();

		void start();
		/// Called automatically on destroy
		void stop();

	private:

		Timer timer;

		vr::IVRSystem* vrSystem = nullptr;

		const vr::TrackingUniverseOrigin universe = vr::TrackingUniverseStanding;

		MyControllerState leftControllerState  { 0 };
		MyControllerState rightControllerState { 1 };

		void processEvents();
		void logEvents();
		void updateButtonsStatus();

		void updatePosition();

		// High-level helpers

		bool getDraggedPoint(const double* & outDragPoint, double & outDragYaw) const;
		bool isDragButtonHeld(const vr::VRControllerState_t & controllerState) const;

		// VR helpers

		vr::IVRSystem * initVrSystem() const;
		bool getIsDragging(vr::TrackedDeviceIndex_t index) const;
		bool getControllerPosition(vr::TrackedDeviceIndex_t controllerIndex) const;

		void setPositionRotation();

};
