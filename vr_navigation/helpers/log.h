#pragma once

#include <string>
#include "../shared/Matrices.h"

void logError(const char* const msg);
void log(const std::string& msg);
void log(const char * const msg);
void log(const Vector3 & pose);

template <class... Args>
void log(const char * const msg, Args&&... args) {
    printf(msg, args...);
    printf("\n");
}

template <class... Args>
void log(const std::string& msg, Args&&... args) {
    printf(msg.c_str(), args...);
    printf("\n");
}

template <typename Msg, class... Args>
void logDebug(const Msg & msg, Args&&... args) {
    // TODO: command line switch --verbose
    // log(msg, args...);
}
