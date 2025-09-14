#include "GuideOval.h"

namespace guide {
bool mostlyInsideGuide(const std::vector<glm::vec2>& pts, float fractionNeeded) {
    if (pts.empty()) return false;
    float ovalCenterX = 1280.0f * 0.5f;
    float ovalCenterY = 720.0f * 0.60f;
    float ovalA = 1280.0f * 0.13f;
    float ovalB = 720.0f * 0.28f;
    int insideCount = 0;
    for (const auto& p : pts) {
        float nx = (p.x - ovalCenterX) / ovalA;
        float ny = (p.y - ovalCenterY) / ovalB;
        if (nx*nx + ny*ny <= 1.f) insideCount++;
    }
    return insideCount >= (int)(fractionNeeded * pts.size());
}
} // namespace guide
