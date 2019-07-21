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

		Matrix4 dragStartTrackingPose;
		/** relative to `dragStartTrackingPose` */
		Vector3 dragStartDragPointPos;
		float dragStartYaw;

		Timer timer;

		vr::IVRSystem* vrSystem = nullptr;

		const vr::TrackingUniverseOrigin universe = vr::TrackingUniverseStanding;
		const vr::EVRButtonId dragButton = vr::k_EButton_Grip;

		// FIXME: find a way to obtain correct index
		MyControllerState leftControllerState  { 3 };
		MyControllerState rightControllerState { 4 };

		void processEvents();
		void doProcessEvents();
		void updateButtonsStatus();

		void updatePosition();
		bool setPositionRotation(const Vector3 & diff);

		// High-level helpers

		/** `outDragPoint` is relative to `getTrackingPose` */
		bool getDraggedPoint(Vector3 & outDragPoint, float & outDragYaw) const;
		bool isDragButtonHeld(const vr::VRControllerState_t & controllerState) const;

		void setDragging(vr::TrackedDeviceIndex_t index, bool dragging);
		bool getIsDragging(vr::TrackedDeviceIndex_t index) const;

};
