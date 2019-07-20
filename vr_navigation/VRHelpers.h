#pragma once

#include <openvr.h>

#include "log.h"

//

vr::IVRSystem * initVrSystem();

// bool getIsDragging(vr::TrackedDeviceIndex_t index);

bool getControllerPosition(
    vr::IVRSystem * vrSystem,
    vr::TrackingUniverseOrigin universe,
    vr::TrackedDeviceIndex_t controllerIndex,
    vr::HmdVector3_t & outPose
);

// Tracking pose get/set

bool getTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    vr::HmdMatrix34_t & outPose
);

bool getTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    vr::HmdMatrix34_t & outPose,
    unsigned & outCollisionBoundsCount,
    vr::HmdQuad_t* & outCollisionBounds
);

//

void setTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    const vr::HmdMatrix34_t & pose,
    const bool doCommitPose = true
);

void setTrackingPose(
    const vr::TrackingUniverseOrigin universe,
    const vr::HmdMatrix34_t & pose,
    vr::HmdQuad_t * & collisionBounds,
    const unsigned collisionBoundsCount
);

//

void commitPose();
