#include "stdafx.h"

#include "log.h"
#include "MyVRStuff.h"

MyVRStuff mvs;

int main()
{
	try {
		log("Hello world!");
		mvs.start();
		log("\nRunning; press [Enter] to stop...");
	} catch (const std::exception& e) {
		logError(e.what());
		getchar();
		return -1;
	}
	getchar();
    return 0;
}
