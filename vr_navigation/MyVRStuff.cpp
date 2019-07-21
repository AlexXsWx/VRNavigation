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

				if (
					!getTrackingPose(
						this->universe,
						this->dragStartTrackingPose,
						this->collisionBounds
					)
				) {
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

	// vr::VRChaperoneSetup()->RevertWorkingCopy();

	Matrix4 pose(this->dragStartTrackingPose);

	Matrix4 poseWithoutTranslation(pose);
	poseWithoutTranslation[12] = 0;
	poseWithoutTranslation[13] = 0;
	poseWithoutTranslation[14] = 0;

	// Rotation

	Matrix4 rotation;
	rotation
		// move to point of rotation
		.translate(-this->dragStartDragPointPosForRot)
		// rotate
		.rotateY(180.0f / M_PI * (dragYaw - this->dragStartYaw))
		// unmove from point of rotation
		.translate(this->dragStartDragPointPosForRot);

	// Translation

	Matrix4 translation;
	translation.translate(
		// add drag-n-drop delta
		// TODO: understand why vector * matrix (as opposed to matrix * vector) works in this case
		(dragPointPos - this->dragStartDragPointPos) * poseWithoutTranslation
	);

	//

	Matrix4 transform = rotation * translation;

	Matrix4 inverseTransform(transform);
	inverseTransform.invert();

	// bounds

	std::vector<vr::HmdQuad_t> collisionBounds(this->collisionBounds);
	for (unsigned i = 0; i < collisionBounds.size(); i++) {
		for (unsigned j = 0; j < 4; j++) {
			Vector4 vec(
				collisionBounds[i].vCorners[j].v[0],
				collisionBounds[i].vCorners[j].v[1],
				collisionBounds[i].vCorners[j].v[2],
				1.0f
			);
			vec = inverseTransform * vec;
			collisionBounds[i].vCorners[j].v[0] = vec[0];
			collisionBounds[i].vCorners[j].v[1] = vec[1];
			collisionBounds[i].vCorners[j].v[2] = vec[2];
		}
	}

	// 0.413597      -1.271854       -1.985742
	// log("Current position: %f\t%f\t%f", curPos[12], curPos[13], curPos[14]);

	setTrackingPose(
		this->universe,
		pose * transform
		// FIXME: this updates actual config files on disk, debounce this or avoid alltogether
		// , collisionBounds
	);
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
