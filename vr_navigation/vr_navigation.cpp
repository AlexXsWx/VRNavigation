#include "stdafx.h"

#include "helpers/log.h"
#include "MyVRStuff.h"

MyVRStuff mvs;

int main() {
    int returnCode = 0;
    try {
        mvs.start();
        log("\nReady; you can now double click & drag a grip button to drag the VR world.");
        log("\nWhen you want to stop, press [Enter] in this window");
    } catch (const std::exception& e) {
        logError(e.what());
        log("\nMake sure steam VR is running and try again.\nPress [Enter] to close this window");
        returnCode = -1;
    }
    getchar();
    return returnCode;
}
