#pragma once

#include <stdint.h>
#include <vector>

struct PmAction
{
public:
    enum Type : int {
        None=0, Scene=1, SceneItem=2, Filter=3, Hotkey=4, FrontEndEvent=5 };
    virtual void execute() = 0;
};

struct PmReaction
{
public:


    uint32_t lingerMs;
    uint32_t cooldownMs;
};
