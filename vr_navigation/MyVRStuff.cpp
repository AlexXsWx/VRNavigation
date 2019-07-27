#include "stdafx.h"
#include "MyVRStuff.h"

// Construct / destroy

MyVRStuff::MyVRStuff() {
	log("MyVRStuff: create");
}

MyVRStuff::~MyVRStuff() {
	log("MyVRStuff: destroy");
	this->stop();
}

// start / stop

void MyVRStuff::start() {

	this->vrSystem = initVrSystem();

	backUpInitial();

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
	log("Standing:");
	this->logTrackingPose(this->initialTrackingPoseStanding);
	log("Seated:");
	this->logTrackingPose(this->initialTrackingPoseSeated);
	log("Collision bounds:");
	this->logCollisionBounds(this->initialCollisionBounds);
}

void MyVRStuff::restoreBackup(bool write) {
	vr::VRChaperoneSetup()->RevertWorkingCopy();
	log("Would restore initial");
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
		vr::VRChaperoneSetup()->HideWorkingSetPreview();
		// restoreBackup(true);
		this->vrSystem = nullptr;
		vr::VR_Shutdown();
	}
}

// Tick

void MyVRStuff::processEvents() {
	this->doProcessEvents();
	if (this->updateButtonsStatus()) {
		this->updatePosition();
	}
}

void MyVRStuff::doProcessEvents() {
	vr::VREvent_t pEvent;
	int debugEventsCount = 0;
	while (this->vrSystem->PollNextEvent(&pEvent, sizeof(pEvent))) {
		if (
			pEvent.trackedDeviceIndex == -1
			&& (
				pEvent.eventType == vr::VREvent_ButtonPress ||
				pEvent.eventType == vr::VREvent_ButtonUnpress
			)
		) {
			log("Ignoring events of device with index -1");
			continue;
		}

		MyControllerState * const state = this->getOrCreateState(pEvent.trackedDeviceIndex);

		std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(
		    std::chrono::system_clock::now().time_since_epoch()
		);
		state->stream.feed({
			now - std::chrono::milliseconds((int)(pEvent.eventAgeSeconds * 1000)),
			pEvent
		});

		bool newDragging = this->isDragging(state->stream);

		if (state->dragging != newDragging) {
			log(
				"Dragging of %i changed: %i -> %i",
				pEvent.trackedDeviceIndex,
				state->dragging,
				newDragging
			);
		}

		state->dragging = newDragging;

		debugEventsCount += 1;
	}

	if (debugEventsCount > 0) {
		// log("that was %i events", debugEventsCount);
	}
}

bool MyVRStuff::isDragging(Stream<WrappedEvent> & stream) const {
	if (stream.empty()) return false;
	auto it = stream.rbegin();
	if (it->event.eventType != vr::VREvent_ButtonPress) return false;
	auto timestamp = it->timestamp;
	unsigned short clicksToGo = 1;
	for (++it; it != stream.rend(); ++it) {
		if (it->event.eventType == vr::VREvent_ButtonPress) {
			if (timestamp - it->timestamp <= this->doubleClickTime) {
				timestamp = it->timestamp;
				if (--clicksToGo == 0) {
					return true;
				}
			} else {
				return false;
			}
		}
	}
	return false;
}

bool MyVRStuff::updateButtonsStatus() {

	bool somethingChanged = false;
	bool somethingDragging = false;
	bool somethingStoppedDragging = false;

	for (
		auto state = this->controllerStates.begin();
		state != this->controllerStates.end();
		state++
	) {
		somethingChanged  = somethingChanged  || state->dragging != state->wasDragging;
		somethingDragging = somethingDragging || state->dragging;
		somethingStoppedDragging = somethingStoppedDragging || (
			state->wasDragging && !state->dragging
		);

		state->wasDragging = state->dragging;
	}

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
		log(this->dragStartDragPointPos);
		log("%.2f", rad2deg(this->dragStartYaw));

		if (
			!getTrackingPose(this->universe, this->dragStartTrackingPose) ||
			!getCollisionBounds(this->dragStartCollisionBounds)
		) {
			log("Failed to getTrackingPose or getCollisionBounds");
			// TODO: handle fail
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
		.rotateY(180.0f / float(M_PI) * (dragYaw - this->dragStartYaw) * this->dragScale)
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

	const auto dragged = filter(
		this->controllerStates,
		[](const auto & state) { return state.dragging; }
	);

	if (dragged.size() <= 0) return false;

	if (dragged.size() == 1) {
		outDragYaw = 0;
		outDragSize = 1;
		return getControllerPosition(this->vrSystem, universe, dragged[0].index, outDragPoint);
	}

	if (dragged.size() > 2) {
		log("Ignoring %i / %i", dragged.size() - 2, dragged.size());
	}
	Vector3 poseOne;
	Vector3 poseTwo;
	const bool succeedOne = getControllerPosition(
		this->vrSystem,
		universe,
		dragged[0].index,
		poseOne
	);
	const bool succeedTwo = getControllerPosition(
		this->vrSystem,
		universe,
		dragged[1].index,
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

MyControllerState * MyVRStuff::getOrCreateState(vr::TrackedDeviceIndex_t index) {

	// Try to find existing

	MyControllerState * state = find(
		this->controllerStates,
		[=](auto arg) { return arg.index == index; }
	);

	if (state != nullptr) return state;

	// If not found, create new one

	this->controllerStates.push_back({
		index,
		false,
		false,
		Stream<WrappedEvent>(
			[this, index](auto value) {
				return (
					value.event.data.controller.button == this->dragButton &&
					value.event.trackedDeviceIndex == index
				);
			},
			[this](auto value, auto all) {
				return (all.back().timestamp - value.timestamp) > 5 * this->doubleClickTime;
			}
		)
	});

	return &(*(this->controllerStates.end() - 1));
}

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
