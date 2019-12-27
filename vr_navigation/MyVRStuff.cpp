#include "stdafx.h"
#include "MyVRStuff.h"

// Helpers

std::chrono::milliseconds timestampFromEventAgeSeconds(float eventAgeSeconds) {
    std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    );
    return (
        now - std::chrono::milliseconds((int)(eventAgeSeconds * 1000))
    );
}

// Construct / destroy

MyVRStuff::MyVRStuff() {
    logDebug("MyVRStuff: create");
}

MyVRStuff::~MyVRStuff() {
    logDebug("MyVRStuff: destroy");
    this->stop();
}

// start / stop

void MyVRStuff::registerHotkeys() {
    registerButtonListener(
        this->dragButton,
        2,
        [this](const auto deviceIndex) {
            if (deviceIndex == -1) {
                logDebug("Ignoring drag button of device with index -1");
                return;
            }
            log("Device %i has now started dragging", deviceIndex);
            this->dragging[deviceIndex] = true;
            // this->getOrCreateState(deviceIndex).dragging = true;
        },
        [this](const auto deviceIndex) {
            log("Device %i has now stopped dragging", deviceIndex);
            this->dragging[deviceIndex] = false;
            // this->getOrCreateState(deviceIndex).dragging = false;
        }
    );

    registerButtonListener(
        this->revertOrToggleUniverseButton,
        1,
        [this](const auto deviceIndex) {
            if (this->isButtonHeld(deviceIndex, this->dragButton, 2)) {
                this->revertWorkingCopy();
            }
        }
    );
}

// FIXME: unsubscribe?
void MyVRStuff::registerButtonListener(
    vr::EVRButtonId button,
    uint8_t clickCount,
    std::function<void(const vr::TrackedDeviceIndex_t & value)> onPress,
    std::function<void(const vr::TrackedDeviceIndex_t & value)> onRelease
) {
    this->buttonListeners.push_back({ button, clickCount, onPress, onRelease });
}

bool MyVRStuff::isButtonHeld(
    vr::TrackedDeviceIndex_t deviceIndex,
    vr::EVRButtonId button,
    uint8_t clickCount
) {
    const auto buttonState = this->controllerStates[deviceIndex].buttonState[button];
    return (
        buttonState.pressed &&
        buttonState.pressCount >= clickCount
    );
}

void MyVRStuff::notifyListeners(
   vr::TrackedDeviceIndex_t deviceIndex,
   vr::EVRButtonId button,
   uint8_t clickCount,
   bool pressed 
) {
    for (
        auto listenerParams = this->buttonListeners.cbegin();
        listenerParams != this->buttonListeners.cend();
        listenerParams++
    ) {
        if (
            listenerParams->button == button &&
            listenerParams->clickCount == clickCount
        ) {
            if (pressed) {
                listenerParams->onPress(deviceIndex);
            } else {
                listenerParams->onRelease(deviceIndex);
            }
        }
    }
}

void MyVRStuff::start() {

    this->vrSystem = initVrSystem();

    backUpInitial();

    registerHotkeys();

    this->timer.setInterval(
        [this]() mutable { this->processEvents(); },
        16
    );
}

// Backup

void MyVRStuff::backUpInitial() {
    getTrackingPose(vr::TrackingUniverseStanding, this->initialTrackingPoseStanding);
    getTrackingPose(vr::TrackingUniverseSeated,   this->initialTrackingPoseSeated);
    getCollisionBounds(this->initialCollisionBounds);

    // debug log
    log("\nInitial configuration:");
    log("\nStanding:\n");
    this->logTrackingPose(this->initialTrackingPoseStanding);
    log("\nSeated:\n");
    this->logTrackingPose(this->initialTrackingPoseSeated);
    log("\nCollision bounds:\n");
    this->logCollisionBounds(this->initialCollisionBounds);
    log("\n");
}

void MyVRStuff::restoreBackup(bool write) {
    vr::VRChaperoneSetup()->RevertWorkingCopy();

    logDebug("Would restore initial");
    return;

    setTrackingPose(vr::TrackingUniverseSeated,   this->initialTrackingPoseSeated);
    setTrackingPose(vr::TrackingUniverseStanding, this->initialTrackingPoseStanding);
    if (write) setCollisionBounds(this->initialCollisionBounds);
    commitPose(write);
}

//

void MyVRStuff::stop() {
    this->timer.stop();

    if (this->vrSystem != nullptr) {
        log("Cleaning up...");
        vr::VRChaperoneSetup()->HideWorkingSetPreview();
        // restoreBackup(true);
        this->vrSystem = nullptr;
        vr::VR_Shutdown();
    }
}

// Tick

void MyVRStuff::processEvents() {
    this->doProcessEvents();
    if (this->updateDragging()) {
        this->updatePosition();
    }
}

void MyVRStuff::doProcessEvents() {
    vr::VREvent_t pEvent;
    int debugEventsCount = 0;
    while (this->vrSystem->PollNextEvent(&pEvent, sizeof(pEvent))) {

        // if (
        //     pEvent.eventType == vr::VREvent_ButtonPress &&
        //     pEvent.data.controller.button == vr::k_EButton_ApplicationMenu &&
        //     some(
        //         this->controllerStates,
        //         [](const auto & state) { return state.dragging; }
        //     )
        // ) {
        //     this->revertWorkingCopy();
        // }

        // if (
        //     pEvent.trackedDeviceIndex == -1
        //     && (
        //         pEvent.eventType == vr::VREvent_ButtonPress ||
        //         pEvent.eventType == vr::VREvent_ButtonUnpress
        //     )
        // ) {
        //     logDebug("Ignoring events of device with index -1");
        //     continue;
        // }

        if (
            pEvent.eventType == vr::VREvent_ButtonPress ||
            pEvent.eventType == vr::VREvent_ButtonUnpress
        ) {
            const auto button = (vr::EVRButtonId)pEvent.data.controller.button;

            // MyControllerState * const controllerState = (
            //     this->getOrCreateState(pEvent.trackedDeviceIndex)
            // );

            const auto timestamp = timestampFromEventAgeSeconds(pEvent.eventAgeSeconds);

            auto & controllerState = this->controllerStates[pEvent.trackedDeviceIndex];
            auto & buttonState = controllerState.buttonState[button];
            if (pEvent.eventType == vr::VREvent_ButtonPress) {
                if (timestamp - buttonState.lastPressTimestamp > this->doubleClickTime) {
                    buttonState.pressCount = 0;
                }
                buttonState.pressed = true;
                buttonState.lastPressTimestamp = timestamp;
                buttonState.pressCount += 1;

                this->notifyListeners(
                    pEvent.trackedDeviceIndex,
                    button,
                    buttonState.pressCount,
                    true
                );
            } else
            if (pEvent.eventType == vr::VREvent_ButtonUnpress) {
                buttonState.pressed = false;
                this->notifyListeners(
                    pEvent.trackedDeviceIndex, button, buttonState.pressCount, false
                );
            }

            // controllerState->stream.feed({ timestamp, pEvent });
        }

        // bool newDragging = this->isDragging(controllerState->stream);

        // if (controllerState->dragging != newDragging) {
        //     log(
        //         "Dragging of %i changed: %i -> %i",
        //         pEvent.trackedDeviceIndex,
        //         controllerState->dragging,
        //         newDragging
        //     );
        // }

        // controllerState->dragging = newDragging;

        debugEventsCount += 1;
    }

    if (debugEventsCount > 0) {
        // log("that was %i events", debugEventsCount);
    }
}

void MyVRStuff::revertWorkingCopy() {
    log("Reverting working copy");
    vr::VRChaperoneSetup()->HideWorkingSetPreview();
    vr::VRChaperoneSetup()->RevertWorkingCopy();
    this->dragging.clear();
    // for (
    //     auto it = this->controllerStates.begin();
    //     it != this->controllerStates.end();
    //     it++
    // ) {
    //     it->dragging = false;
    //     it->stream.clear();
    // }
}

// bool MyVRStuff::isDragging(Stream<WrappedEvent> & stream) const {
//     if (stream.empty()) return false;
//     auto it = stream.rbegin();
//     if (it->event.eventType != vr::VREvent_ButtonPress) return false;
//     auto timestamp = it->timestamp;
//     unsigned short clicksToGo = 1;
//     for (++it; it != stream.rend(); ++it) {
//         if (it->event.eventType == vr::VREvent_ButtonPress) {
//             if (timestamp - it->timestamp <= this->doubleClickTime) {
//                 timestamp = it->timestamp;
//                 if (--clicksToGo == 0) {
//                     return true;
//                 }
//             } else {
//                 return false;
//             }
//         }
//     }
//     return false;
// }

bool MyVRStuff::updateDragging() {

    bool somethingChanged = false;
    bool somethingDragging = false;
    bool somethingStoppedDragging = false;

    for (
        auto it = this->dragging.begin();
        it != this->dragging.end();
        it++
    ) {
        const auto dragging = it->second;
        const auto wasDragging = this->draggingLast[it->first];
        somethingChanged  = somethingChanged  || dragging != wasDragging;
        somethingDragging = somethingDragging || dragging;
        somethingStoppedDragging = somethingStoppedDragging || (wasDragging && !dragging);
    }
    this->draggingLast = this->dragging;

    if (!somethingChanged) return somethingDragging;

    log("Dragging changed: dragging = %i", somethingDragging);

    if (!somethingDragging) {
        this->dragScale = 1.0f;
        log("New drag scale: %.2f", this->dragScale);
        return false;
    }

    if (somethingStoppedDragging) {
        this->dragScale *= this->dragSize / this->dragStartSize;
        log("New drag scale: %.2f", this->dragScale);
    }

    const bool succeed = this->getDraggedPoint(
        this->dragStartDragPointPos,
        this->dragStartYaw,
        this->dragStartSize
    );
    this->dragSize = this->dragStartSize;
    float whatever;
    // FIXME: handle fail
    this->getDraggedPoint(this->dragStartDragPointPosForRot, whatever, whatever, false);
    if (succeed) {
        if (
            !getTrackingPose(this->universe, this->dragStartTrackingPose) ||
            !getCollisionBounds(this->dragStartCollisionBounds)
        ) {
            logError("Failed to getTrackingPose or getCollisionBounds");
            // TODO: handle fail
            return false;
        }
    }

    return true;
}

void MyVRStuff::updatePosition() {

    Vector3 dragPointPos;
    float dragYaw;
    bool dragNDropIsActive = this->getDraggedPoint(dragPointPos, dragYaw, this->dragSize);
    if (!dragNDropIsActive) return;

    // vr::VRChaperoneSetup()->RevertWorkingCopy();

    Matrix4 pose(this->dragStartTrackingPose);

    Matrix4 poseWithoutTranslation(pose);
    poseWithoutTranslation[12] = 0;
    poseWithoutTranslation[13] = 0;
    poseWithoutTranslation[14] = 0;

    // Rotation

    Matrix4 rotation;
    rotation
        // move to point of rotation
        .translate(-this->dragStartDragPointPosForRot)
        // rotate
        .rotateY(
            (float)rad2deg(dragYaw - this->dragStartYaw) *
            clamp(this->dragScale, 0.25f, 2.0f)
        )
        // unmove from point of rotation
        .translate(this->dragStartDragPointPosForRot);

    // Translation

    Matrix4 translation;
    translation.translate(
        // add drag-n-drop delta
        // TODO: understand why vector * matrix (as opposed to matrix * vector) works in this case
        (dragPointPos - this->dragStartDragPointPos) * this->dragScale * poseWithoutTranslation
    );

    //

    Matrix4 transform = rotation * translation;

    Matrix4 inverseTransform(transform);
    inverseTransform.invert();

    // bounds

    std::vector<vr::HmdQuad_t> collisionBounds(this->dragStartCollisionBounds);
    for (auto it = collisionBounds.begin(); it != collisionBounds.end(); it++) {
        for (unsigned i = 0; i < 4; i++) {
            Vector4 vec(
                it->vCorners[i].v[0],
                it->vCorners[i].v[1],
                it->vCorners[i].v[2],
                1.0f
            );
            vec = inverseTransform * vec;
            it->vCorners[i].v[0] = vec[0];
            it->vCorners[i].v[1] = vec[1];
            it->vCorners[i].v[2] = vec[2];
        }
    }

    // 0.413597      -1.271854       -1.985742
    // log("Current position: %f\t%f\t%f", curPos[12], curPos[13], curPos[14]);

    setTrackingPose(this->universe, pose * transform);
    // FIXME: this updates actual config files on disk, debounce this or avoid alltogether
    // setCollisionBounds(collisionBounds);
    commitPose();
}

// High-level helpers

bool MyVRStuff::getDraggedPoint(
    Vector3 & outDragPoint,
    float & outDragYaw,
    float & outDragSize,
    bool absolute
) const {
    const vr::TrackingUniverseOrigin universe = (
        absolute ? vr::TrackingUniverseRawAndUncalibrated : this->universe
    );

    std::vector<vr::TrackedDeviceIndex_t> dragged;
    for (auto it = this->dragging.cbegin(); it != this->dragging.cend(); it++) {
        if (it->second) dragged.push_back(it->first);
    }

    if (dragged.size() <= 0) return false;

    if (dragged.size() == 1) {
        outDragYaw = 0;
        outDragSize = 1;
        return getControllerPosition(this->vrSystem, universe, dragged[0], outDragPoint);
    }

    if (dragged.size() > 2) {
        logDebug("Ignoring %i / %i", dragged.size() - 2, dragged.size());
    }
    Vector3 poseOne;
    Vector3 poseTwo;
    const bool succeedOne = getControllerPosition(
        this->vrSystem,
        universe,
        dragged[0],
        poseOne
    );
    const bool succeedTwo = getControllerPosition(
        this->vrSystem,
        universe,
        dragged[1],
        poseTwo
    );
    if (!succeedOne || !succeedTwo) return false;
    outDragSize = poseOne.distance(poseTwo);
    lerp(poseOne, poseTwo, 0.5, outDragPoint);
    outDragYaw = atan2(
        poseTwo[0] - poseOne[0],
        poseTwo[2] - poseOne[2]
    );
    return true;
}

// MyControllerState * MyVRStuff::getOrCreateState(vr::TrackedDeviceIndex_t index) {

//     // Try to find existing

//     MyControllerState * state = find(
//         this->controllerStates,
//         [=](const auto & arg) { return arg.index == index; }
//     );

//     if (state != nullptr) return state;

//     // If not found, create new one

//     this->controllerStates.push_back({
//         index,
//         false,
//         false,
//         Stream<WrappedEvent>(
//             [this, index](const auto & value) {
//                 return (
//                     value.event.data.controller.button == this->dragButton &&
//                     value.event.trackedDeviceIndex == index && (
//                         value.event.eventType == vr::VREvent_ButtonPress ||
//                         value.event.eventType == vr::VREvent_ButtonUnpress
//                     )
//                 );
//             },
//             [this](const auto & value, const auto & all) {
//                 return (all.back().timestamp - value.timestamp) > 5 * this->doubleClickTime;
//             }
//         )
//     });

//     return &(*(this->controllerStates.end() - 1));
// }

//

void MyVRStuff::logTrackingPose(Matrix4 & m) {
    log(
        std::string("%.4f\t%.4f\t%.4f\t%.4f\n") +
        std::string("%.4f\t%.4f\t%.4f\t%.4f\n") +
        std::string("%.4f\t%.4f\t%.4f\t%.4f\n") +
        std::string("%.4f\t%.4f\t%.4f\t%.4f"),
        m[0], m[4], m[8],  m[12],
        m[1], m[5], m[9],  m[13],
        m[2], m[6], m[10], m[14],
        m[3], m[7], m[11], m[15]
    );
}

void MyVRStuff::logCollisionBounds(std::vector<vr::HmdQuad_t> & v) {
    for (auto it = v.begin(); it != v.end(); it++) {
        for (unsigned i = 0; i < 4; i++) {
            log(
                "%.2f\t%.2f\t%.2f",
                it->vCorners[i].v[0],
                it->vCorners[i].v[1],
                it->vCorners[i].v[2]
            );
        }
    }
}
