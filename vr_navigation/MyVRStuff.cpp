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

	// log("would start VR stuff"); return;

	auto initError = vr::VRInitError_None;
	this->vrSystem = vr::VR_Init(&initError, vr::VRApplication_Background);
	if (initError != vr::VRInitError_None) {
		if (
			initError == vr::VRInitError_Init_HmdNotFound ||
			initError == vr::VRInitError_Init_HmdNotFoundPresenceFailed
		) {
			logError("Could not find HMD!");
		} else
		if (initError == vr::VRInitError_Init_NoServerForBackgroundApp) {
			logError("SteamVR is not running");
		}
		throw std::runtime_error(
			std::string("Failed to initialize OpenVR: ") +
			std::string(vr::VR_GetVRInitErrorAsEnglishDescription(initError))
		);
	}
	this->vrInitialized = true;

	this->timer.setInterval(
		[this]() mutable { this->onTick(); },
		1000
	);
}

void MyVRStuff::stop() {
	this->timer.stop();

	if (this->vrInitialized && this->vrSystem != nullptr) {
		vr::VR_Shutdown();
		this->vrSystem = nullptr;
	}
}

// Tick

void MyVRStuff::onTick() {
	log("tick");
	this->printDebugInfo();
	this->checkButtonsChange();
	return;
	this->updatePosition();
}

void MyVRStuff::printDebugInfo() {
	vr::VREvent_t pEvent;
	int counter = 0;
	while (this->vrSystem->PollNextEvent(&pEvent, sizeof(pEvent))) {
		if (
			pEvent.eventType == vr::VREvent_ButtonPress ||
			pEvent.eventType == vr::VREvent_ButtonUnpress
		) {
			log("button state changed");
		}
		counter += 1;
	}
	if (counter > 0) {
		log("that was %i events", counter);
	}
}

void MyVRStuff::checkButtonsChange() {

	vr::VRControllerState_t controllerState;
	bool getStateSucceed;

	// Left

	getStateSucceed = this->vrSystem->GetControllerState(
		this->leftControllerState.index,
		&controllerState,
		1
	);
	const bool leftDragging = getStateSucceed ? this->isDragButtonHeld(controllerState) : false;

	// Right

	getStateSucceed = this->vrSystem->GetControllerState(
		this->rightControllerState.index,
		&controllerState,
		1
	);
	const bool rightDragging = getStateSucceed ? this->isDragButtonHeld(controllerState) : false;

	//

	if (
		leftDragging  != this->leftControllerState.dragging ||
		rightDragging != this->rightControllerState.dragging
	) {
		log(
			"drag changed\n%i: %i -> %i\n%i: %i -> %i",
			this->leftControllerState.index,  this->leftControllerState.dragging,  leftDragging,
			this->rightControllerState.index, this->rightControllerState.dragging, rightDragging
		);
		// this->getDraggedPoint(
			// this->dragStartPos,
			// this->dragStartYaw
		// );
	}

	this->leftControllerState.dragging  = leftDragging;
	this->rightControllerState.dragging = rightDragging;
}

void MyVRStuff::updatePosition() {
	if (
		!this->leftControllerState.dragging &&
		!this->rightControllerState.dragging
	) {
		return;
	}

	const double * dragPoint;
	double dragYaw;
	bool dragNDropIsActive = this->getDraggedPoint(dragPoint, dragYaw);
	if (!dragNDropIsActive) return;

	this->setPositionRotation();

	delete dragPoint;
	dragPoint = nullptr;
}

// Helpers

bool MyVRStuff::getDraggedPoint(
	const double* & outDragPoint,
	double & outDragYaw
) {
	return false;
}

bool MyVRStuff::getControllerPosition(vr::TrackedDeviceIndex_t controllerIndex) const {
	vr::VRControllerState_t controllerState;
	vr::TrackedDevicePose_t pose;
	bool succeed = this->vrSystem->GetControllerStateWithPose(
		this->universe,
		controllerIndex,
		&controllerState, 1,
		&pose
	);
	if (!succeed) return false;

	return true;
}

bool MyVRStuff::isDragButtonHeld(const vr::VRControllerState_t & controllerState) const {
	return 0 != (
		controllerState.ulButtonPressed &
		vr::ButtonMaskFromId(vr::k_EButton_Grip)
	);
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
		vr::VRChaperoneSetup()->GetWorkingCollisionBoundsInfo(collisionBounds, &collisionBoundsCount);
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
		vr::VRChaperoneSetup()->SetWorkingCollisionBoundsInfo(collisionBounds, collisionBoundsCount);
		delete collisionBounds;
	}

	vr::VRChaperoneSetup()->CommitWorkingCopy(vr::EChaperoneConfigFile_Live);
}
