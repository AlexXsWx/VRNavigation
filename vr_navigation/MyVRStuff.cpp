#include "stdafx.h"
#include "MyVRStuff.h"

// Construct / destroy

MyVRStuff::MyVRStuff() {
	log("MyVRStuff: create");
}

MyVRStuff::~MyVRStuff() {
	log("MyVRStuff: destroy");
	this->stop();
}

// start / stop

void MyVRStuff::start() {

	this->vrSystem = initVrSystem();

	this->timer.setInterval(
		[this]() mutable { this->processEvents(); },
		16
	);
}

void MyVRStuff::stop() {
	this->timer.stop();

	if (this->vrSystem != nullptr) {
		vr::VRChaperoneSetup()->HideWorkingSetPreview();
		this->vrSystem = nullptr;
		vr::VR_Shutdown();
	}
}

// Tick

void MyVRStuff::processEvents() {
	// log("tick");
	this->doProcessEvents();
	this->updateButtonsStatus();
	this->updatePosition();
}

void MyVRStuff::doProcessEvents() {
	vr::VREvent_t pEvent;
	int counter = 0;
	while (this->vrSystem->PollNextEvent(&pEvent, sizeof(pEvent))) {
		if (
			pEvent.eventType == vr::VREvent_ButtonPress ||
			pEvent.eventType == vr::VREvent_ButtonUnpress
		) {
			log(
				"button state changed, %i: %i",
				pEvent.trackedDeviceIndex,
				pEvent.data.controller.button == this->dragButton
			);
			if (
				pEvent.trackedDeviceIndex != -1 &&
				pEvent.data.controller.button == this->dragButton
			) {
				this->setDragging(
					pEvent.trackedDeviceIndex,
					pEvent.eventType == vr::VREvent_ButtonPress
				);
			}
		}
		counter += 1;
	}
	// if (counter > 0) {
	// 	log("that was %i events", counter);
	// }
}

void MyVRStuff::updateButtonsStatus() {

	const bool leftDragging  = this->getIsDragging(this->leftControllerState.index);
	const bool rightDragging = this->getIsDragging(this->rightControllerState.index);

	if (
		leftDragging  != this->leftControllerState.wasDragging ||
		rightDragging != this->rightControllerState.wasDragging
	) {
		log(
			"drag changed\n%i: %i -> %i\n%i: %i -> %i",
			this->leftControllerState.index,  this->leftControllerState.wasDragging,  leftDragging,
			this->rightControllerState.index, this->rightControllerState.wasDragging, rightDragging
		);

		this->leftControllerState.wasDragging  = leftDragging;
		this->rightControllerState.wasDragging = rightDragging;

		if (leftDragging || rightDragging) {
			const bool succeed = this->getDraggedPoint(
				this->dragStartDragPointPos,
				this->dragStartYaw
			);
			float whatever;
			// FIXME: handle fail
			this->getDraggedPoint(this->dragStartDragPointPosForRot, whatever, false);
			if (succeed) {
				log(this->dragStartDragPointPos);
				log("%.2f", rad2deg(this->dragStartYaw));

				if (!getTrackingPose(this->universe, this->dragStartTrackingPose)) {
					log("Failed to getTrackingPose");
					// TODO: handle fail
				}
			}
		}
	}
}

void MyVRStuff::updatePosition() {
	if (
		!this->leftControllerState.dragging &&
		!this->rightControllerState.dragging
	) {
		return;
	}

	Vector3 dragPointPos;
	float dragYaw;
	bool dragNDropIsActive = this->getDraggedPoint(dragPointPos, dragYaw);
	if (!dragNDropIsActive) return;

	Matrix4 pose(this->dragStartTrackingPose);

	Matrix4 poseWithoutTranslation(pose);
	poseWithoutTranslation[12] = 0;
	poseWithoutTranslation[13] = 0;
	poseWithoutTranslation[14] = 0;

	// Rotation

	Matrix4 rotation;
	rotation.translate(
		// move to point of rotation
		// Vector3(
		// 	this->dragStartTrackingPose[12]
		// 	this->dragStartTrackingPose[13]
		// 	this->dragStartTrackingPose[14]
		// )
		- this->dragStartDragPointPosForRot
	).rotateY(
		// rotate
		// FIXME: because this is relative, the rotation  is not one-to-one
		180.0f / M_PI * (dragYaw - this->dragStartYaw)
	).translate(
		// unmove from point of rotation
		this->dragStartDragPointPosForRot
		// - Vector3(
		// 	this->dragStartTrackingPose[12]
		// 	this->dragStartTrackingPose[13]
		// 	this->dragStartTrackingPose[14]
		// )
	);

	// Translation

	Matrix4 translation;
	translation.translate(
		// add drag-n-drop delta
		// FIXME: because this is relative, the translation is not one-to-one
		(dragPointPos - this->dragStartDragPointPos) * poseWithoutTranslation
	);

	// FIXME: transform chaperone boundaries too
	setTrackingPose(this->universe, pose * rotation * translation);

	// this->setPositionRotation(diff);
}

bool MyVRStuff::setPositionRotation(const Vector3 & diff) {

	// vr::VRChaperoneSetup()->RevertWorkingCopy();

	Matrix4 curPos;
	unsigned collisionBoundsCount = 0;
	vr::HmdQuad_t* collisionBounds = nullptr;

	if (!getTrackingPose(this->universe, curPos/*, collisionBoundsCount, collisionBounds*/)) {
		return false;
	}

	// 0.413597      -1.271854       -1.985742
	log("Current position: %f\t%f\t%f", curPos[12], curPos[13], curPos[14]);

	// return false;

	curPos[12] += diff[0];
	curPos[13] += diff[1];
	curPos[14] += diff[2];

	// const float offsetX = 0.0f; // std::cos(angle)
	// const float offsetY = 0.0f;
	// const float offsetZ = 0.0f; // std::sin(angle)

	// curPos.m[0][3] += curPos.m[0][0] * offsetX;
	// curPos.m[1][3] += curPos.m[1][0] * offsetX;
	// curPos.m[2][3] += curPos.m[2][0] * offsetX;

	// curPos.m[0][3] += curPos.m[0][1] * offsetY;
	// curPos.m[1][3] += curPos.m[1][1] * offsetY;
	// curPos.m[2][3] += curPos.m[2][1] * offsetY;

	// curPos.m[0][3] += curPos.m[0][2] * offsetZ;
	// curPos.m[1][3] += curPos.m[1][2] * offsetZ;
	// curPos.m[2][3] += curPos.m[2][2] * offsetZ;

	// FIXME: init?
	// vr::HmdMatrix34_t rotMat;
	// for (unsigned b = 0; b < collisionBoundsCount; b++) {
	// 	for (unsigned c = 0; c < 4; c++) {
	// 		auto& corner = collisionBounds[b].vCorners[c];
	// 		vr::HmdVector3_t newVal;
	// 		// FIXME: set newVal value
	// 		// corner = newVal;
	// 	}
	// }

	setTrackingPose(this->universe, curPos/*, collisionBounds, collisionBoundsCount*/);

	if (collisionBounds != nullptr) {
		delete[] collisionBounds;
	}

	return true;
}

// High-level helpers

bool MyVRStuff::getDraggedPoint(
	Vector3 & outDragPoint,
	float & outDragYaw,
	bool absolute
) const {
	const vr::TrackingUniverseOrigin universe = (
		absolute ? vr::TrackingUniverseRawAndUncalibrated : this->universe
	);
	if (this->leftControllerState.dragging && !this->rightControllerState.dragging) {
		outDragYaw = 0;
		return getControllerPosition(
			this->vrSystem,
			universe,
			this->leftControllerState.index,
			outDragPoint
		);
	} else
	if (!this->leftControllerState.dragging && this->rightControllerState.dragging) {
		outDragYaw = 0;
		return getControllerPosition(
			this->vrSystem,
			universe,
			this->rightControllerState.index,
			outDragPoint
		);
	} else
	if (this->leftControllerState.dragging && this->rightControllerState.dragging) {
		Vector3 leftPose;
		Vector3 rightPose;
		const bool leftSucceed = getControllerPosition(
			this->vrSystem,
			universe,
			this->leftControllerState.index,
			leftPose
		);
		const bool rightSucceed = getControllerPosition(
			this->vrSystem,
			universe,
			this->rightControllerState.index,
			rightPose
		);
		if (!leftSucceed || !rightSucceed) return false;
		lerp(leftPose, rightPose, 0.5, outDragPoint);
		outDragYaw = atan2(
			rightPose[0] - leftPose[0],
			rightPose[2] - leftPose[2]
		);
		return true;
	}

	return false;
}

bool MyVRStuff::isDragButtonHeld(const vr::VRControllerState_t & controllerState) const {
	return 0 != (
		controllerState.ulButtonPressed &
		vr::ButtonMaskFromId(this->dragButton)
	);
}

void MyVRStuff::setDragging(vr::TrackedDeviceIndex_t index, bool dragging) {
	if (this->leftControllerState.index == index) {
		this->leftControllerState.dragging = dragging;
	} else
	if (this->rightControllerState.index == index) {
		this->rightControllerState.dragging = dragging;
	} else {
		log("Unexpected index %i", index);
	}
}

bool MyVRStuff::getIsDragging(vr::TrackedDeviceIndex_t index) const {
	if (this->leftControllerState.index == index) {
		return this->leftControllerState.dragging;
	} else
	if (this->rightControllerState.index == index) {
		return this->rightControllerState.dragging;
	} else {
		log("Unexpected index %i", index);
		return false;
	}
}
