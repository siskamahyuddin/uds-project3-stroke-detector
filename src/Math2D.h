#pragma once
#include "ofMain.h"
#include <cmath>

namespace math2d {
    inline glm::vec2 rotateAround(const glm::vec2& p, const glm::vec2& c, float rad) {
        glm::vec2 t = p - c;
        float cs = std::cos(rad), sn = std::sin(rad);
        return glm::vec2(cs * t.x - sn * t.y, sn * t.x + cs * t.y) + c;
    }
    inline float interOcular(const glm::vec2& L, const glm::vec2& R) {
        return std::max(1.0f, glm::distance(L, R));
    }
} // namespace math2d
