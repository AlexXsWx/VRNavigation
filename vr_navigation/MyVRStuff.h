#pragma once

#include <vector>
#include <map>
#include <algorithm>
#include <math.h>
#include <chrono>
#include <openvr.h>

#include "helpers/math.h"
#include "helpers/log.h"
#include "helpers/async.h"
#include "shared/Matrices.h"
#include "helpers/VRHelpers.h"
// #include "helpers/vectorHelpers.h"
// #include "helpers/Stream.h"

//

// TODO: check if this is actually needed for the bool type it was added for
template<typename T, T defaultValue>
struct MapDefaultValueType {
    MapDefaultValueType () : value(T(defaultValue)) {}
    MapDefaultValueType (T const & value) : value(value) {}
    operator T & () { return value; }
    operator T const & () const { return value; }
    T value;
};

// struct WrappedEvent {
//     std::chrono::milliseconds timestamp;
//     vr::VREvent_t event;
// };

// struct MyControllerState {
//     vr::TrackedDeviceIndex_t index;
//     bool dragging    = false;
//     bool wasDragging = false;
//     Stream<WrappedEvent> stream;
// }

struct MyButtonState {
    std::chrono::milliseconds lastPressTimestamp {0};
    bool pressed = false;
    uint8_t pressCount = 0;
};

struct MyControllerState {
    std::map<vr::EVRButtonId, MyButtonState> buttonState;
};

struct ButtonListenerParams {
    vr::EVRButtonId button;
    uint8_t clickCount;
    std::function<void(const vr::TrackedDeviceIndex_t & value)> onPress;
    std::function<void(const vr::TrackedDeviceIndex_t & value)> onRelease;
};

//

// TODO: allow to rotate with 1 controller
// TODO: persistent storage
// TODO: config files
// TODO: log how frequent the file updates
class MyVRStuff {

    public:

        MyVRStuff();
        ~MyVRStuff();

        void start();
        /// Called automatically on destroy
        void stop();

    private:

        // Params
        const vr::EVRButtonId dragButton                   = vr::k_EButton_Grip;
        const vr::EVRButtonId revertOrToggleUniverseButton = vr::k_EButton_ApplicationMenu;
        const std::chrono::milliseconds  doubleClickTime {250};

        //

        vr::TrackingUniverseOrigin universe = vr::TrackingUniverseStanding;

        vr::IVRSystem* vrSystem = nullptr;

        Timer timer;

        // Input

        std::map<
            vr::TrackedDeviceIndex_t,
            MyControllerState
        > controllerStates;
        // std::vector<MyControllerState> controllerStates;
        // MyControllerState * getOrCreateState(vr::TrackedDeviceIndex_t index);

        std::vector<ButtonListenerParams> buttonListeners;

        void registerButtonListener(
            vr::EVRButtonId button,
            uint8_t clickCount,
            std::function<void(const vr::TrackedDeviceIndex_t & value)> onPress,
            std::function<void(const vr::TrackedDeviceIndex_t & value)> onRelease
                // TODO: clean up
                = [](const auto deviceIndex){}
        );

        bool isButtonHeld(
            vr::TrackedDeviceIndex_t deviceIndex,
            vr::EVRButtonId button,
            uint8_t clickCount
        );

        void notifyListeners(
           vr::TrackedDeviceIndex_t deviceIndex,
           vr::EVRButtonId button,
           uint8_t clickCount,
           bool pressed 
        );

        // Initial

        std::vector<vr::HmdQuad_t> initialCollisionBounds;
        Matrix4 initialTrackingPoseStanding;
        Matrix4 initialTrackingPoseSeated;

        void backUpInitial();
        void restoreBackup(bool write = false);

        // Drag start

        std::vector<vr::HmdQuad_t> dragStartCollisionBounds;
        Matrix4 dragStartTrackingPose;
        Vector3 dragStartDragPointPos;
        // TODO: find a way to calculate this out of `dragStartDragPointPos`
        Vector3 dragStartDragPointPosForRot;
        float dragStartYaw = 0.0f;
        float dragStartSize = 1.0f;

        // Drag update

        float dragScale = 1.0f;
        float dragSize  = 1.0f;

        std::map<vr::TrackedDeviceIndex_t, MapDefaultValueType<bool, false>> dragging;
        std::map<vr::TrackedDeviceIndex_t, MapDefaultValueType<bool, false>> draggingLast;

        //

        void registerHotkeys();

        // bool isDragging(Stream<WrappedEvent> & stream) const;

        void revertWorkingCopy();

        void processEvents();
        void doProcessEvents();

        bool updateDragging();
        void updatePosition();

        bool getDraggedPoint(
            Vector3 & outDragPoint,
            float & outDragYaw,
            float & outDragSize,
            bool absolute = true
        ) const;

        //

        void logTrackingPose(Matrix4 & m);
        void logCollisionBounds(std::vector<vr::HmdQuad_t> & v);

};
