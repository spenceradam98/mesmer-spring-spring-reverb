// Factory presets. Names are descriptive only and refer to no third-party product.
#pragma once

#include "Engine.h"
#include <vector>

namespace ms
{
struct Preset
{
    const char* name;
    const char* category;
    Params p;
};

const std::vector<Preset>& factoryPresets();
} // namespace ms
