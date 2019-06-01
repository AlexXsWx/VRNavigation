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
}

// #include "async.h"
/* std::atomic_bool cancelToken;
setInterval(cancelToken, 1000, []() {
printf("ping");
});
getchar();
cancelToken.store(false);*/

void MyVRStuff::start() {

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