## About Pixel Match Switcher
When streaming games, OBS can auto switch the scene configuration based on the currently active window title.. However, it is often desirable to switch the scene configuration based on what part of the game is being displayed in the UI, even when the window title is always the same.

Two reasons this are useful are. (a) streamers often like to have displays with a bigger streamer-cam and/or ads when they are in a game menu, or some other “non gameplay” area of the game. (b) streamers often need to *hide* certain elements of game content, so they don’t get stream-sniped by people who want to gain an advantage by watching their stream while playing against them.  Currently, streamers handle these cases manually, by manually switching scenes everytime, either with the OBS UI or by assigning hotkeys.

**Pixel Match Switcher** plugin makes this scene switching automatic, by comparing match images against regions of video. Match images can be extracted by users directly from the video feed. Match regions are ordered by priority

## :construction: User Resources
- [Alpha Build Installers of OBS Fork + Plugin](https://github.com/HoneyHazard/PixelMatchSwitcher/releases)
- [User Manual](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual)
  - [Pixel Match Filter](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual#pixel-match-filter)
  - [UI Overview](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual#ui-elements-overview)
  - [Basic concepts](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual#basic-concepts)
      + [Match Entry](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual#match-entry)
      + [Match Image](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual#match-image)
      + [Match Parameters](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual#match-parameters)
      + [Match Image Capture](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual#match-image-capture)
      + [Target Scene](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual#target-scene)
      + [Match Result](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual#match-result)
      + [Match List](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual#match-list)
      + [Preset](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual#preset)
      + [No-match Target](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual#no-match-target)
  - [Automask Walkthrough](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual#automask-walkthrough)
  - [UI Details](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/User-Manual#ui-details)

## :construction: Build Instructions
- [Windows: standalone plugin](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/Build-on-Windows:-Standalone-Plugin)
- \[**TODO**\] Windows: OBS In-tree
- [Linux: standalone plugin](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/Build-on-Linux:-Standalone-Plugin)
- [Debian-based Linux: OBS In-tree](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/Build-on-Debian-based-Linux:-OBS-In-Tree)

## Screenshot
![image](https://raw.githubusercontent.com/wiki/HoneyHazard/PixelMatchSwitcher/images/readme_screenshot.png)

## Planned Features
- Automatically choose a rules preset based on which scene(s) are active
- Save/Load presets configuration to/from XML
- Download game presets from an online database
