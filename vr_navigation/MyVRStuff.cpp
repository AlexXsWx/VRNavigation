#include "stdafx.h"
#include "MyVRStuff.h"

MyVRStuff::MyVRStuff() {
	log("MyVRStuff: create");
}

MyVRStuff::~MyVRStuff() {
	log("MyVRStuff: destroy");
	if (this->vrInitialized) {
		vr::VR_Shutdown();
		this->vrSystem = nullptr;
	}

	this->timer.stop();
}

void MyVRStuff::start() {

	int counter = 3;
	this->timer.setInterval([&, counter]() mutable {
		counter -= 1;
		if (counter == 0) this->timer.stop();
		log("ping %i", counter);
	}, 1000);

	log("would start VR stuff"); return;

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

	//

	// if (!vr::VROverlay()) {
	//     logError("Is OpenVR running?");
	//     throw std::runtime_error(std::string("No Overlay interface"));
	// }
}

void MyVRStuff::updatePosition() {

	vr::VRChaperoneSetup()->RevertWorkingCopy();

	const auto universe = vr::TrackingUniverseStanding;

	unsigned collisionBoundsCount = 0;
	vr::HmdQuad_t* collisionBounds = nullptr;
	vr::VRChaperoneSetup()->GetWorkingCollisionBoundsInfo(nullptr, &collisionBoundsCount);

	vr::HmdMatrix34_t curPos;
	if (universe == vr::TrackingUniverseStanding) {
		vr::VRChaperoneSetup()->GetWorkingStandingZeroPoseToRawTrackingPose(&curPos);
	} else {
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

	if (universe == vr::TrackingUniverseStanding) {
		vr::VRChaperoneSetup()->SetWorkingStandingZeroPoseToRawTrackingPose(&curPos);
	} else {
		vr::VRChaperoneSetup()->SetWorkingSeatedZeroPoseToRawTrackingPose(&curPos);
	}

	if (collisionBounds != nullptr) {
		vr::VRChaperoneSetup()->SetWorkingCollisionBoundsInfo(collisionBounds, collisionBoundsCount);
		delete collisionBounds;
	}

	vr::VRChaperoneSetup()->CommitWorkingCopy(vr::EChaperoneConfigFile_Live);

}
