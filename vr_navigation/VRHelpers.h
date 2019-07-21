#pragma once

#include <openvr.h>

#include "log.h"
#include "shared/Matrices.h"

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

bool getTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    Matrix4 & outPose,
    unsigned & outCollisionBoundsCount,
    vr::HmdQuad_t* & outCollisionBounds
);

//

void setTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    const Matrix4 & pose,
    const bool doCommitPose = true
);

void setTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    const Matrix4 & pose,
    vr::HmdQuad_t * & collisionBounds,
    const unsigned collisionBoundsCount
);

//

void commitPose();
