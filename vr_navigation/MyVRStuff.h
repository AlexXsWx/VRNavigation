#pragma once

#include <vector>
#include <algorithm>
#include <math.h>
#include <openvr.h>

#include "math.h"
#include "log.h"
#include "async.h"
#include "shared/Matrices.h"
#include "VRHelpers.h"

//

struct MyControllerState {
	vr::TrackedDeviceIndex_t index;
	bool dragging = false;
	bool wasDragging = false;
};

//

// TODO: enter drag on double-click
// TODO: allow to rotate with 1 controller
// TODO: a way to reset
// TODO: a way to scale factor
// TODO: save initial settings
// TODO: persistent storage
// TODO: config files
class MyVRStuff {

	public:

		MyVRStuff();
		~MyVRStuff();

		void start();
		/// Called automatically on destroy
		void stop();

	private:

		std::vector<vr::HmdQuad_t> collisionBounds;
		Matrix4 dragStartTrackingPose;
		Vector3 dragStartDragPointPos;
		// TODO: find a way to calculate this out of `dragStartDragPointPos`
		Vector3 dragStartDragPointPosForRot;
		float dragStartYaw;

		Timer timer;

		vr::IVRSystem* vrSystem = nullptr;

		const vr::TrackingUniverseOrigin universe = vr::TrackingUniverseStanding;
		const vr::EVRButtonId dragButton = vr::k_EButton_Grip;

		std::vector<MyControllerState> controllerStates;

		void processEvents();
		void doProcessEvents();
		void updateButtonsStatus();

		void updatePosition();

		// High-level helpers

		bool getDraggedPoint(
			Vector3 & outDragPoint,
			float & outDragYaw,
			bool absolute = true
		) const;
		bool isDragButtonHeld(const vr::VRControllerState_t & controllerState) const;

		void setDragging(vr::TrackedDeviceIndex_t index, bool dragging);

};
