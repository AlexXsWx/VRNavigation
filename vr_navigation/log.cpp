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