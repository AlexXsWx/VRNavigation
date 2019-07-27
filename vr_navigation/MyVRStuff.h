#pragma once

#include <vector>
#include <algorithm>
#include <math.h>
#include <ctime>
#include <openvr.h>

#include "helpers/math.h"
#include "helpers/log.h"
#include "helpers/async.h"
#include "shared/Matrices.h"
#include "helpers/VRHelpers.h"
#include "helpers/vectorHelpers.h"
#include "helpers/Stream.h"

//

struct WrappedEvent {
	std::time_t timestamp;
	vr::VREvent_t event;
};

struct MyControllerState {
	vr::TrackedDeviceIndex_t index;
	bool dragging    = false;
	bool wasDragging = false;
	Stream<WrappedEvent> stream;
};

//

// TODO: allow to rotate with 1 controller
// TODO: a way to reset
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

		// Params

		const vr::TrackingUniverseOrigin universe        = vr::TrackingUniverseStanding;
		const vr::EVRButtonId            dragButton      = vr::k_EButton_Grip;
		const float                      doubleClickTime = 0.25f;

		//

		vr::IVRSystem* vrSystem = nullptr;

		Timer timer;

		// Initial

		std::vector<vr::HmdQuad_t> initialCollisionBounds;
		Matrix4 initialTrackingPoseStanding;
		Matrix4 initialTrackingPoseSeated;

		void MyVRStuff::backUpInitial();
		void MyVRStuff::restoreBackup(bool write = false);

		// Drag start

		std::vector<vr::HmdQuad_t> dragStartCollisionBounds;
		Matrix4 dragStartTrackingPose;
		Vector3 dragStartDragPointPos;
		// TODO: find a way to calculate this out of `dragStartDragPointPos`
		Vector3 dragStartDragPointPosForRot;
		float dragStartYaw = 0.0f;
		float dragStartSize = 1.0f;

		// Drag update

		float dragScale = 1.0f;
		float dragSize  = 1.0f;

		std::vector<MyControllerState> controllerStates;
		MyControllerState * getOrCreateState(vr::TrackedDeviceIndex_t index);

		//

		bool isDragging(Stream<WrappedEvent> & stream) const;

		void processEvents();
		void doProcessEvents();
		bool updateButtonsStatus();
		void updatePosition();

		bool getDraggedPoint(
			Vector3 & outDragPoint,
			float & outDragYaw,
			float & outDragSize,
			bool absolute = true
		) const;

};
