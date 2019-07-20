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
		// log(
		// 	"drag changed\n%i: %i -> %i\n%i: %i -> %i",
		// 	this->leftControllerState.index,  this->leftControllerState.wasDragging,  leftDragging,
		// 	this->rightControllerState.index, this->rightControllerState.wasDragging, rightDragging
		// );

		this->leftControllerState.wasDragging  = leftDragging;
		this->rightControllerState.wasDragging = rightDragging;

		if (leftDragging || rightDragging) {
			const bool succeed = this->getDraggedPoint(
				this->dragStartPos,
				this->dragStartYaw
			);
			// if (succeed) {
			// 	log(this->dragStartPos);
			// 	log("%.2f", rad2deg(this->dragStartYaw));
			// }
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

	vr::HmdVector3_t dragPoint;
	float dragYaw;
	bool dragNDropIsActive = this->getDraggedPoint(dragPoint, dragYaw);
	if (!dragNDropIsActive) return;

	vr::HmdVector3_t diff;

	diff.v[0] = dragPoint.v[0] - this->dragStartPos.v[0];
	diff.v[1] = dragPoint.v[1] - this->dragStartPos.v[1];
	diff.v[2] = dragPoint.v[2] - this->dragStartPos.v[2];

	// log("position delta: %.2f\t%.2f\t%.2f", diff.v[0], diff.v[1], diff.v[2]);
	// log("yaw delta: %.2f", rad2deg(dragYaw - this->dragStartYaw));

	this->setPositionRotation(diff);
	// FIXME: don't change `dragStartPos` here
	this->dragStartPos.v[0] = dragPoint.v[0];
	this->dragStartPos.v[1] = dragPoint.v[1];
	this->dragStartPos.v[2] = dragPoint.v[2];
}

bool MyVRStuff::setPositionRotation(const vr::HmdVector3_t & diff) {

	// vr::VRChaperoneSetup()->RevertWorkingCopy();

	vr::HmdMatrix34_t curPos;
	unsigned collisionBoundsCount = 0;
	vr::HmdQuad_t* collisionBounds = nullptr;

	if (!getTrackingPose(this->universe, curPos/*, collisionBoundsCount, collisionBounds*/)) {
		return false;
	}

	// 0.413597      -1.271854       -1.985742
	log("Current position: %f\t%f\t%f", curPos.m[0][3], curPos.m[1][3], curPos.m[2][3]);

	// return false;

	curPos.m[0][3] += diff.v[0];
	curPos.m[1][3] += diff.v[1];
	curPos.m[2][3] += diff.v[2];

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
	vr::HmdVector3_t & outDragPoint,
	float & outDragYaw
) const {
	if (this->leftControllerState.dragging && !this->rightControllerState.dragging) {
		outDragYaw = 0;
		return getControllerPosition(
			this->vrSystem,
			this->universe,
			this->leftControllerState.index,
			outDragPoint
		);
	} else
	if (!this->leftControllerState.dragging && this->rightControllerState.dragging) {
		outDragYaw = 0;
		return getControllerPosition(
			this->vrSystem,
			this->universe,
			this->rightControllerState.index,
			outDragPoint
		);
	} else
	if (this->leftControllerState.dragging && this->rightControllerState.dragging) {
		vr::HmdVector3_t leftPose;
		vr::HmdVector3_t rightPose;
		const bool leftSucceed = getControllerPosition(
			this->vrSystem,
			this->universe,
			this->leftControllerState.index,
			leftPose
		);
		const bool rightSucceed = getControllerPosition(
			this->vrSystem,
			this->universe,
			this->rightControllerState.index,
			rightPose
		);
		if (!leftSucceed || !rightSucceed) return false;
		lerp(leftPose, rightPose, 0.5, outDragPoint);
		outDragYaw = atan2(
			rightPose.v[0] - leftPose.v[0],
			rightPose.v[2] - leftPose.v[2]
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
