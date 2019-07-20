#pragma once

#include <openvr.h>
#include <math.h>

#include "log.h"
#include "async.h"
#include "math.h"

//

struct MyControllerState {
	vr::TrackedDeviceIndex_t index;
	bool dragging = false;
	bool wasDragging = false;
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

		vr::HmdVector3_t dragStartPos;
		float dragStartYaw;

		Timer timer;

		vr::IVRSystem* vrSystem = nullptr;

		const vr::TrackingUniverseOrigin universe = vr::TrackingUniverseStanding;

		// FIXME: find a way to obtain correct index
		MyControllerState leftControllerState  { 3 };
		MyControllerState rightControllerState { 4 };

		void processEvents();
		void doProcessEvents();
		void updateButtonsStatus();

		void updatePosition();

		// High-level helpers

		bool getDraggedPoint(vr::HmdVector3_t & outDragPoint, float & outDragYaw) const;
		bool isDragButtonHeld(const vr::VRControllerState_t & controllerState) const;

		void setDragging(vr::TrackedDeviceIndex_t index, bool dragging);
		bool getIsDragging(vr::TrackedDeviceIndex_t index) const;

		// VR helpers

		vr::IVRSystem * initVrSystem() const;
		bool getControllerPosition(
			vr::TrackedDeviceIndex_t controllerIndex,
			vr::HmdVector3_t & outPose
		) const;

		bool setPositionRotation(const vr::HmdVector3_t & diff);

		bool MyVRStuff::test(float deltaY);

};
