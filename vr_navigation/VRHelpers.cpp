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
    Vector3 & outPose
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

    outPose[0] = pose.mDeviceToAbsoluteTracking.m[0][3];
    outPose[1] = pose.mDeviceToAbsoluteTracking.m[1][3];
    outPose[2] = pose.mDeviceToAbsoluteTracking.m[2][3];

    return true;
}

// Tracking pose get/set

bool getTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    Matrix4 & outPose
) {
    const auto chaperone = vr::VRChaperoneSetup();

    vr::HmdMatrix34_t pose;

    if (universe == vr::TrackingUniverseStanding) {
        const bool succeed = chaperone->GetWorkingStandingZeroPoseToRawTrackingPose(&pose);
        if (!succeed) {
            log("Failed to GetWorkingStandingZeroPoseToRawTrackingPose");
            return false;
        }
    } else
    if (universe == vr::TrackingUniverseSeated) {
        const bool succeed = chaperone->GetWorkingSeatedZeroPoseToRawTrackingPose(&pose);
        if (!succeed) {
            log("Failed to GetWorkingSeatedZeroPoseToRawTrackingPose");
            return false;
        }
    } else {
        log("Unsupported universe");
        return false;
    }

    outPose.set(
        pose.m[0][0], pose.m[1][0], pose.m[2][0], 0.0f,
        pose.m[0][1], pose.m[1][1], pose.m[2][1], 0.0f,
        pose.m[0][2], pose.m[1][2], pose.m[2][2], 0.0f,
        pose.m[0][3], pose.m[1][3], pose.m[2][3], 1.0f
    );

    return true;
}

bool getTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    Matrix4 & outPose,
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
    const Matrix4 & pose,
    const bool doCommitPose
) {
    vr::HmdMatrix34_t hmdPose;
    hmdPose.m[0][0] = pose[0];
    hmdPose.m[1][0] = pose[1];
    hmdPose.m[2][0] = pose[2];
    hmdPose.m[0][1] = pose[4];
    hmdPose.m[1][1] = pose[5];
    hmdPose.m[2][1] = pose[6];
    hmdPose.m[0][2] = pose[8];
    hmdPose.m[1][2] = pose[9];
    hmdPose.m[2][2] = pose[10];
    hmdPose.m[0][3] = pose[12];
    hmdPose.m[1][3] = pose[13];
    hmdPose.m[2][3] = pose[14];

    const auto chaperone = vr::VRChaperoneSetup();
    if (universe == vr::TrackingUniverseStanding) {
        chaperone->SetWorkingStandingZeroPoseToRawTrackingPose(&hmdPose);
    } else
    if (universe == vr::TrackingUniverseSeated) {
        chaperone->SetWorkingSeatedZeroPoseToRawTrackingPose(&hmdPose);
    } else {
        log("Unsupported universe");
        return;
    }
    if (doCommitPose) commitPose();
}

void setTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    const Matrix4 & pose,
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
