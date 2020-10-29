## About Pixel Match Switcher

**Pixel Match Switcher** plugin for [OBS Studio](https://projectobs.com/en/) allows scene switching to be triggered by comparing match images against regions of video. A match image can trigger transition to a specific scene, and several match entries can be arranged in a list of decreasing priority. Considerable amount of options is available for customizing the matching parameters and the switching behavior. Significant effort has been made towards a quick and user-friendly creation of match rules.

The following are some of the foreseeable use cases for the plugin:
- Streamers often like to have displays with a bigger streamer-cam and/or ads when they are in a game menu, or some other “non gameplay” area of the game.
- Streamers often need to *hide* certain elements of game content, so they don’t get stream-sniped by people who want to gain an advantage by watching their stream while playing against them. Currently, streamers handle these cases manually, by manually switching scenes everytime, either with the OBS UI or by assigning hotkeys.
- Tournament broadcasters may wish to provide video overlays to identify participants and their team, or to show the game score. These could be activated or hidden automatically, in reaction to specific in-game graphics.
- Generally speaking, any use case where a region of a video frame will contain predictable pixels that should trigger a scene transition.

Presently, the plugin requires a special [atomic-effects](#Atomic-Effects-Fork-of-OBS) fork of OBS to function. We provide installers of the forked OBS with the plugin included to make everything easier to try out.

## Screenshot
![image](https://raw.githubusercontent.com/wiki/HoneyHazard/PixelMatchSwitcher/images/readme_screenshot.png)

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

## Atomic Effects Fork of OBS
Pixel Match Switcher plugin requires **effect results** and **atomic counters** to work. These precursor features are not present in the mainline of OBS Studio at the moment of writing. We provide the [atomic-effects](https://github.com/HoneyHazard/obs-studio-atomic-effects) fork to introduce these key features into OBS and make our plugin possible. Effort is being made to keep the fork up-to-date with the latest OBS commits.

We hope to see the [changes](https://github.com/HoneyHazard/obs-studio-atomic-effects/wiki/Overview-of-the-changes-introduced-in-the-atomic-effects-fork-of-obs-studio) of the atomic-effects fork eventually integrated into OBS mainline. Perhaps, the Pixel Match Switcher itself could one day be bundled with OBS; part of the starter arsenal to make the awesome tool even more awesome. Our hearts would be filled with joy when the gift of pixel match switching becomes easily accessible to the community.

## Planned Expansion Features
- Allow show/hide of an image/mask source as an alternative to scene switching
- Automatically choose a rules preset based on which scene(s) are active
- Save/load presets configuration to/from XML
- Download game presets from an online database
- Advanced matching logic: inverse matching, AND, OR, etc

## Thought-About Expansion Features
- Try to match an image anywhere in the video frame (not just at a fixed position) and allow show/hide of an image/mask source at the location where the match image would be found
- More advanced image processing could be made available for identifying elements in the video, so long as it is fast (and should probably stick to being implemented in the shader). This could broaden possible uses of the plugin.

## :construction: Build Resources
- [Windows: standalone plugin](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/Build-on-Windows:-Standalone-Plugin)
- \[**TODO**\] Windows: OBS In-tree
- [Linux: standalone plugin](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/Build-on-Linux:-Standalone-Plugin)
- [Debian-based Linux: OBS In-tree](https://github.com/HoneyHazard/PixelMatchSwitcher/wiki/Build-on-Debian-based-Linux:-OBS-In-Tree)
- [obs-studio-pixel-match-switcher](https://github.com/PixelMatchSwitcher/obs-studio-pixel-match-switcher) is forked from [obs-studio-atomic-effects](https://github.com/HoneyHazard/obs-studio-atomic-effects), for the sole purpose of easy, in-tree builds of the plugin. These builds become [installer releases](https://github.com/HoneyHazard/PixelMatchSwitcher/releases) of Pixel Match Switcher.

## Related Github Projects
- [obs-studio-atomic-effects](https://github.com/HoneyHazard/obs-studio-atomic-effects)
- [SceneSwitcher](https://github.com/WarmUpTill/SceneSwitcher)
- [obs-screenshot-plugin](https://github.com/synap5e/obs-screenshot-plugin)
