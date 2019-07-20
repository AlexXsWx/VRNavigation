#pragma once

#include <string>
#include <openvr.h>

void logError(const char* const msg);
void log(const std::string& msg);
void log(const char * const msg);
void log(const vr::HmdVector3_t & pose);

template <class... Args>
void log(const char * const msg, Args&&... args) {
	printf(msg, args...);
	printf("\n");
}
