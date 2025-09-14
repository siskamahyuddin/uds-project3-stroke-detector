#pragma once
#include "ofMain.h"

class StabilityMonitor {
public:
    float move_thresh_norm = 0.012f; // normalized by IOD
    void reset();
    void update(const glm::vec2& center, float iod, bool inside_guide, float dt);
    float stableTime() const;
private:
    glm::vec2 prev_center_{0,0};
    bool      have_prev_center_ = false;
    float     stable_time_ = 0.f;
};
