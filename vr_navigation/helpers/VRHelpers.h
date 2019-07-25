#pragma once

#include <vector>
#include <openvr.h>

#include "log.h"
#include "../shared/Matrices.h"

//

vr::IVRSystem * initVrSystem();

// bool getIsDragging(vr::TrackedDeviceIndex_t index);

bool getControllerPosition(
    vr::IVRSystem * vrSystem,
    vr::TrackingUniverseOrigin universe,
    vr::TrackedDeviceIndex_t controllerIndex,
    Vector3 & outPose
);

// Tracking pose get/set

bool getTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    Matrix4 & outPose
);

void setTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    const Matrix4 & pose,
    const bool callCommitPose = false
);

// Collision bounds get/set

bool getCollisionBounds(std::vector<vr::HmdQuad_t> & collisionBoundsVector);

void setCollisionBounds(
    const std::vector<vr::HmdQuad_t> & collisionBoundsVector,
    const bool callCommitPose = false
);

//

void commitPose(bool write = false);
