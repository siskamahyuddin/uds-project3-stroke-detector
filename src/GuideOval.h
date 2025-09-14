#pragma once
#include "ofMain.h"
#include <vector>

namespace guide {
bool mostlyInsideGuide(const std::vector<glm::vec2>& pts, float fractionNeeded = 0.92f);
} // namespace guide
