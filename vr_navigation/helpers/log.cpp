#include "stdafx.h"
#include "log.h"

void logError(const char * const msg) {
    log(
        std::string("Error: ") +
        std::string(msg)
    );
}

void log(const std::string& msg) {
    log(msg.c_str());
}

void log(const char * const msg) {
    printf(msg);
    printf("\n");
}

void log(const Vector3 & pose) {
    log("%.2f\t%.2f\t%.2f", pose[0], pose[1], pose[2]);
}
