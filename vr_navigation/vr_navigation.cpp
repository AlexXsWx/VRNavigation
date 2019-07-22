#include "stdafx.h"

#include "helpers/log.h"
#include "MyVRStuff.h"

MyVRStuff mvs;

int main()
{
	int returnCode = 0;
	try {
		log("Hello world!");
		mvs.start();
		log("\nRunning; press [Enter] to stop...");
	} catch (const std::exception& e) {
		logError(e.what());
		returnCode = -1;
	}
	getchar();
    return returnCode;
}
