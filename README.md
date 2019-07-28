Video: [https://www.youtube.com/watch?v=NERh8rTF5Nk](https://www.youtube.com/watch?v=NERh8rTF5Nk)

# Disclaimer

**Use at your own risk.**

I recommend doing a backup of your steam/config folder since the software is changing configurations that may be written on disk by other applications and thus be set as default configuration.  
The software attempts to only set the configuration temporarily and reset it before it terminates (when you close it via [Enter]), but this feature is still in development and might not work properly; also the configuration is shared with other applications so when they decide to save it to disk it they will also save the temporary changes.

# Description

This application is designed to let user adjust VR play space while in VR.

# How to use

1. Run steam VR
2. (Optionally) run your VR game
3. Run the program ([releases](https://github.com/AlexXsWx/VRNavigation/releases)) and leave it running
4. Make sure to close the program (via Enter) before quitting any other VR application or the changes you've done might be set as new "default".

# Controls

Double-click & hold grip button to start dragging. Release to stop dragging.  
Drag with two controllers to rotate world.  
Adjust distance between two dragged controllers and release one of them to scale movement / rotation speed. Release last controller to reset the movement / rotation speed scaling back to 1.  
Press Menu while dragging to reset movemenet / rotation.  

# Known issues

**Chaperone boundaries are currently not updated.** and are being dragged with the virtual world.  
If you significantly tune up the movement/rotation speed scaling, the whole VR world is gonna jitter.
