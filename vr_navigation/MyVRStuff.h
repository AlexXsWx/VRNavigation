#pragma once

#include <openvr.h>
#include <math.h>

#include "log.h"
#include "async.h"
#include "math.h"
#include "VRHelpers.h"
#include "shared/Matrices.h"

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

		vr::HmdMatrix34_t dragStartPose;
		vr::HmdVector3_t dragStartDragPointPos;
		float dragStartYaw;

		Timer timer;

		vr::IVRSystem* vrSystem = nullptr;

		const vr::TrackingUniverseOrigin universe = vr::TrackingUniverseStanding;
		const vr::EVRButtonId dragButton = vr::k_EButton_Grip;

		// FIXME: find a way to obtain correct index
		MyControllerState leftControllerState  { 1 };
		MyControllerState rightControllerState { 2 };

		void processEvents();
		void doProcessEvents();
		void updateButtonsStatus();

		void updatePosition();
		bool setPositionRotation(const vr::HmdVector3_t & diff);

		// High-level helpers

		bool getDraggedPoint(vr::HmdVector3_t & outDragPoint, float & outDragYaw) const;
		bool isDragButtonHeld(const vr::VRControllerState_t & controllerState) const;

		void setDragging(vr::TrackedDeviceIndex_t index, bool dragging);
		bool getIsDragging(vr::TrackedDeviceIndex_t index) const;

};
