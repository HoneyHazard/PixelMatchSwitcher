## PixelMatchSwitcher
PixelMatchSwitcher plugin for OBS Studio. Planned to remain private until published.

https://docs.google.com/document/d/1k657WGIwOCGDUhERAb_LpTHWAqHb9IHX43TfSKo6f5A/edit

## Prerequisites
- Clone OBS studio (atomic effects fork): https://github.com/HoneyHazard/obs-studio-atomic-effects/
- Checkout `atomic-effects` branch
- Follow instructions to configure, generate, build and install* OBS Studio: https://github.com/obsproject/obs-studio/wiki/Install-Instructions

## Build Instructions (Linux)
- Clone the PixelMatchSwitcher repository: https://github.com/HoneyHazard/PixelMatchSwitcher
- Use cmake, cmake-gui, or any editor that works with CMakeLists.txt to configure
- [*] If you have installed OBS in the previous section, the following variables may have been autodetected. If not, you could configure these manually to point to your clone and build of the OBS atomic effects fork
```
LIBOBS_INCLUDE_DIR=<obs clone location>/libobs
LIBOBS_FRONTEND_INCLUDE_DIR=<obs clone location>/UI/obs-frontend-api/
LIBOBS_UI_DIR=<obs clone location>/UI
LIBOBS_LIB=<obs build location>/libobs/libobs.so
LIBOBS_FRONTEND_API_LIB=<obs build location>/UI/obs-frontend-api/libobs-frontend-api.so
```
- Finish configure, generate. build and install*

## Run instructions
- [*] You shouldn't need to do anything special to run if you have performed install of the OBS studio (atomic effects fork) and PixelMatchSwitcher plugin
- TODO: instructions for referencing directly to OBS fork workspace if install was not performed

## Install Instructions (Linux)
```
cd build
sudo make install
```

## Use Instructions
TODO
