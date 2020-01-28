## PixelMatchSwitcher
PixelMatchSwitcher plugin for OBS Studio. Planned to remain private until published.

https://docs.google.com/document/d/1k657WGIwOCGDUhERAb_LpTHWAqHb9IHX43TfSKo6f5A/edit

## Prerequisites
- Clone OBS studio: https://github.com/obsproject/obs-studio
- Follow instructions to configure, generate, build and install OBS Studio: https://github.com/obsproject/obs-studio/wiki/Install-Instructions

## Build Instructions (Debian-based Linux)
- Clone the PixelMatchSwitcher repository
- Use cmake, cmake-gui, or any editor that works with CMakeLists.txt to configure
- Set the variables, if not auto-configured:
```
LIBOBS_FRONTEND_API_LIB=/usr/local/lib/libobs-frontend-api.so
LIBOBS_FRONTEND_INCLUDE_DIR=/home/user/obs-studio/UI/obs-frontend-api/
LIBOBS_INCLUDE_DIR=/home/user/obs-studio/libobs
LIBOBS_LIB=/usr/local/lib/libobs.so
```

## Install Instructions (Linux)
```
cd build
sudo make install
```

## Use Instructions
TODO
