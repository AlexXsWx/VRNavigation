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

void setTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    const Matrix4 & pose,
    const bool callCommitPose
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
    if (callCommitPose) commitPose();
}

// Collision bounds get/set

bool getCollisionBounds(std::vector<vr::HmdQuad_t> & collisionBoundsVector) {
    const auto chaperone = vr::VRChaperoneSetup();

    collisionBoundsVector.clear();

    unsigned collisionBoundsCount = 0;

    chaperone->GetWorkingCollisionBoundsInfo(
        nullptr,
        &collisionBoundsCount
    );
    
    if (collisionBoundsCount > 0) {
        vr::HmdQuad_t* collisionBounds = new vr::HmdQuad_t[collisionBoundsCount];
        const bool succeed = chaperone->GetWorkingCollisionBoundsInfo(
            collisionBounds, &collisionBoundsCount
        );
        if (succeed) {
            for (unsigned i = 0; i < collisionBoundsCount; i++) {
                collisionBoundsVector.push_back(collisionBounds[i]);
            }
        } else {
            log("Failed to GetWorkingCollisionBoundsInfo");
            return false;
        }
        delete[] collisionBounds;
        collisionBounds = nullptr;
    }

    return true;
}

void setCollisionBounds(
    const std::vector<vr::HmdQuad_t> & collisionBoundsVector,
    const bool callCommitPose
) {
    const unsigned collisionBoundsCount = static_cast<unsigned>(collisionBoundsVector.size());
    if (collisionBoundsCount > 0) {
        vr::HmdQuad_t* collisionBounds = new vr::HmdQuad_t[collisionBoundsCount];
        for (unsigned i = 0; i < collisionBoundsCount; i++) {
            collisionBounds[i] = collisionBoundsVector[i];
        }
        vr::VRChaperoneSetup()->SetWorkingCollisionBoundsInfo(
            collisionBounds, collisionBoundsCount
        );
        delete[] collisionBounds;
    }
    if (callCommitPose) commitPose();
}

//

void commitPose(bool write) {
    const auto chaperone = vr::VRChaperoneSetup();

    if (!write) {
        chaperone->ShowWorkingSetPreview();
        return;
    }

    log("Commiting working copy...");
    if (chaperone->CommitWorkingCopy(vr::EChaperoneConfigFile_Live)) {
        log("Success");
    } else {
        log("Failed to CommitWorkingCopy");
    }
}
