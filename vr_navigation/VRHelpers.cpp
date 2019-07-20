#include "stdafx.h"
#include "VRHelpers.h"

vr::IVRSystem* initVrSystem() {
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
// bool getIsDragging(vr::TrackedDeviceIndex_t index) {
//  vr::VRControllerState_t controllerState;
//  bool getStateSucceed = this->vrSystem->GetControllerState(index, &controllerState, 1);
//  return getStateSucceed ? this->isDragButtonHeld(controllerState) : false;
// }

bool getControllerPosition(
    vr::IVRSystem * vrSystem,
    vr::TrackingUniverseOrigin universe,
    vr::TrackedDeviceIndex_t controllerIndex,
    vr::HmdVector3_t & outPose
) {
    vr::VRControllerState_t controllerState;
    vr::TrackedDevicePose_t pose;
    bool succeed = vrSystem->GetControllerStateWithPose(
        universe,
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

// Tracking pose get/set

bool getTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    vr::HmdMatrix34_t & outPose
) {
    const auto chaperone = vr::VRChaperoneSetup();

    if (universe == vr::TrackingUniverseStanding) {
        const bool succeed = chaperone->GetWorkingStandingZeroPoseToRawTrackingPose(&outPose);
        if (!succeed) {
            log("Failed to GetWorkingStandingZeroPoseToRawTrackingPose");
            return false;
        }
    } else {
        const bool succeed = chaperone->GetWorkingSeatedZeroPoseToRawTrackingPose(&outPose);
        if (!succeed) {
            log("Failed to GetWorkingSeatedZeroPoseToRawTrackingPose");
            return false;
        }
    }

    return true;
}

bool getTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    vr::HmdMatrix34_t & outPose,
    unsigned & outCollisionBoundsCount,
    vr::HmdQuad_t* & outCollisionBounds
) {
    const auto chaperone = vr::VRChaperoneSetup();

    outCollisionBoundsCount = 0;

    const bool succeed = chaperone->GetWorkingCollisionBoundsInfo(
        nullptr,
        &outCollisionBoundsCount
    );
    if (!succeed) {
        log("Failed to GetWorkingCollisionBoundsInfo");
        outCollisionBoundsCount = 0;
        // return false;
    }
    
    if (outCollisionBoundsCount > 0) {
        outCollisionBounds = new vr::HmdQuad_t[outCollisionBoundsCount];
        const bool succeed = chaperone->GetWorkingCollisionBoundsInfo(
            outCollisionBounds, &outCollisionBoundsCount
        );
        if (!succeed) {
            log("Failed to GetWorkingCollisionBoundsInfo");
            outCollisionBoundsCount = 0;
            delete[] outCollisionBounds;
            outCollisionBounds = nullptr;
            // return false;
        }
    }

    return getTrackingPose(universe, outPose);
}

//

void setTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    const vr::HmdMatrix34_t & pose,
    const bool doCommitPose
) {
    const auto chaperone = vr::VRChaperoneSetup();
    if (universe == vr::TrackingUniverseStanding) {
        chaperone->SetWorkingStandingZeroPoseToRawTrackingPose(&pose);
    } else {
        chaperone->SetWorkingSeatedZeroPoseToRawTrackingPose(&pose);
    }
    if (doCommitPose) commitPose();
}

void setTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    const vr::HmdMatrix34_t & pose,
    vr::HmdQuad_t * & collisionBounds,
    const unsigned collisionBoundsCount
) {
    setTrackingPose(universe, pose, false);

    if (collisionBounds != nullptr) {
        vr::VRChaperoneSetup()->SetWorkingCollisionBoundsInfo(
            collisionBounds, collisionBoundsCount
        );
    };

    commitPose();
}

//

void commitPose() {
    const auto chaperone = vr::VRChaperoneSetup();

    // if (chaperone->CommitWorkingCopy(vr::EChaperoneConfigFile_Live)) {
    //  return true;
    // } else {
    //  log("Failed to CommitWorkingCopy");
    //  return false;
    // }

    chaperone->ShowWorkingSetPreview();
}
