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

	this->vrSystem = this->initVrSystem();

	this->timer.setInterval(
		[this]() mutable { this->processEvents(); },
		16
	);
}

void MyVRStuff::stop() {
	this->timer.stop();

	if (this->vrSystem != nullptr) {
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
				pEvent.data.controller.button == vr::k_EButton_Grip
			);
			if (
				pEvent.trackedDeviceIndex != -1 &&
				pEvent.data.controller.button == vr::k_EButton_Grip
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
				this->dragStartPos,
				this->dragStartYaw
			);
			if (succeed) {
				log(this->dragStartPos);
				log("%.2f", rad2deg(this->dragStartYaw));
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

	vr::HmdVector3_t dragPoint;
	float dragYaw;
	bool dragNDropIsActive = this->getDraggedPoint(dragPoint, dragYaw);
	if (!dragNDropIsActive) return;

	vr::HmdVector3_t diff;

	diff.v[0] = dragPoint.v[0] - this->dragStartPos.v[0];
	diff.v[1] = dragPoint.v[1] - this->dragStartPos.v[1];
	diff.v[2] = dragPoint.v[2] - this->dragStartPos.v[2];

	log("position delta: %.2f\t%.2f\t%.2f", diff.v[0], diff.v[1], diff.v[2]);
	log("yaw delta: %.2f", rad2deg(dragYaw - this->dragStartYaw));

	// this->setPositionRotation();
}

// High-level helpers

bool MyVRStuff::getDraggedPoint(
	vr::HmdVector3_t & outDragPoint,
	float & outDragYaw
) const {
	if (this->leftControllerState.dragging && !this->rightControllerState.dragging) {
		outDragYaw = 0;
		return this->getControllerPosition(this->leftControllerState.index, outDragPoint);
	} else
	if (!this->leftControllerState.dragging && this->rightControllerState.dragging) {
		outDragYaw = 0;
		return this->getControllerPosition(this->rightControllerState.index, outDragPoint);
	} else
	if (this->leftControllerState.dragging && this->rightControllerState.dragging) {
		vr::HmdVector3_t leftPose;
		vr::HmdVector3_t rightPose;
		const bool leftSucceed  = this->getControllerPosition(this->leftControllerState.index,  leftPose);
		const bool rightSucceed = this->getControllerPosition(this->rightControllerState.index, rightPose);
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
		vr::ButtonMaskFromId(vr::k_EButton_Grip)
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

// ================================================================================================
//                                           VR Helpers
// ================================================================================================

vr::IVRSystem* MyVRStuff::initVrSystem() const {
	auto initError = vr::VRInitError_None;

	auto vrSystem = vr::VR_Init(&initError, vr::VRApplication_Background);
	if (initError == vr::VRInitError_None) {
		return vrSystem;
	}

	// Handle failure
	if (
		initError == vr::VRInitError_Init_HmdNotFound ||
		initError == vr::VRInitError_Init_HmdNotFoundPresenceFailed
	) {
		logError("Could not find HMD!");
	}
	else
	if (initError == vr::VRInitError_Init_NoServerForBackgroundApp) {
		logError("SteamVR is not running");
	}
	throw std::runtime_error(
		std::string("Failed to initialize OpenVR: ") +
		std::string(vr::VR_GetVRInitErrorAsEnglishDescription(initError))
	);
}

// Doesn't seem to work - probably because is deprecated
// bool MyVRStuff::getIsDragging(vr::TrackedDeviceIndex_t index) const {
// 	vr::VRControllerState_t controllerState;
// 	bool getStateSucceed = this->vrSystem->GetControllerState(index, &controllerState, 1);
// 	return getStateSucceed ? this->isDragButtonHeld(controllerState) : false;
// }

bool MyVRStuff::getControllerPosition(
	vr::TrackedDeviceIndex_t controllerIndex,
	vr::HmdVector3_t & outPose
) const {
	vr::VRControllerState_t controllerState;
	vr::TrackedDevicePose_t pose;
	bool succeed = this->vrSystem->GetControllerStateWithPose(
		this->universe,
		controllerIndex,
		&controllerState, 1,
		&pose
	);
	if (!succeed) return false;

	outPose.v[0] = pose.mDeviceToAbsoluteTracking.m[0][3];
	outPose.v[1] = pose.mDeviceToAbsoluteTracking.m[1][3];
	outPose.v[2] = pose.mDeviceToAbsoluteTracking.m[2][3];

	return true;
}

void MyVRStuff::setPositionRotation() {
	vr::VRChaperoneSetup()->RevertWorkingCopy();

	unsigned collisionBoundsCount = 0;
	vr::HmdQuad_t* collisionBounds = nullptr;
	vr::VRChaperoneSetup()->GetWorkingCollisionBoundsInfo(nullptr, &collisionBoundsCount);

	vr::HmdMatrix34_t curPos;
	if (this->universe == vr::TrackingUniverseStanding) {
		vr::VRChaperoneSetup()->GetWorkingStandingZeroPoseToRawTrackingPose(&curPos);
	}
	else {
		vr::VRChaperoneSetup()->GetWorkingSeatedZeroPoseToRawTrackingPose(&curPos);
	}
	if (collisionBoundsCount > 0) {
		collisionBounds = new vr::HmdQuad_t[collisionBoundsCount];
		vr::VRChaperoneSetup()->GetWorkingCollisionBoundsInfo(
			collisionBounds, &collisionBoundsCount
		);
	}

	const float offsetX = 0.0f; // std::cos(angle)
	const float offsetY = 0.0f;
	const float offsetZ = 0.0f; // std::sin(angle)

	curPos.m[0][3] += curPos.m[0][0] * offsetX;
	curPos.m[1][3] += curPos.m[1][0] * offsetX;
	curPos.m[2][3] += curPos.m[2][0] * offsetX;

	curPos.m[0][3] += curPos.m[0][1] * offsetY;
	curPos.m[1][3] += curPos.m[1][1] * offsetY;
	curPos.m[2][3] += curPos.m[2][1] * offsetY;

	curPos.m[0][3] += curPos.m[0][2] * offsetZ;
	curPos.m[1][3] += curPos.m[1][2] * offsetZ;
	curPos.m[2][3] += curPos.m[2][2] * offsetZ;

	// FIXME: init?
	vr::HmdMatrix34_t rotMat;
	for (unsigned b = 0; b < collisionBoundsCount; b++) {
		for (unsigned c = 0; c < 4; c++) {
			auto& corner = collisionBounds[b].vCorners[c];
			vr::HmdVector3_t newVal;
			// FIXME: set newVal value
			// corner = newVal;
		}
	}

	if (this->universe == vr::TrackingUniverseStanding) {
		vr::VRChaperoneSetup()->SetWorkingStandingZeroPoseToRawTrackingPose(&curPos);
	}
	else {
		vr::VRChaperoneSetup()->SetWorkingSeatedZeroPoseToRawTrackingPose(&curPos);
	}

	if (collisionBounds != nullptr) {
		vr::VRChaperoneSetup()->SetWorkingCollisionBoundsInfo(
			collisionBounds, collisionBoundsCount
		);
		delete collisionBounds;
	}

	vr::VRChaperoneSetup()->CommitWorkingCopy(vr::EChaperoneConfigFile_Live);
}
