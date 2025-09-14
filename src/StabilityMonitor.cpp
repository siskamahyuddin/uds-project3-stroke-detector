#include "StabilityMonitor.h"

void StabilityMonitor::reset() {
    have_prev_center_ = false;
    stable_time_ = 0.f;
}
void StabilityMonitor::update(const glm::vec2& center, float iod, bool inside_guide, float dt) {
    if (!inside_guide) {
        have_prev_center_ = false;
        stable_time_ = 0.f;
        return;
    }
    if (!have_prev_center_) {
        prev_center_ = center;
        have_prev_center_ = true;
        stable_time_ = 0.f;
        return;
    }
    float move_norm = glm::distance(center, prev_center_) / std::max(1.f, iod);
    prev_center_ = center;
    if (move_norm < move_thresh_norm) stable_time_ += dt;
    else stable_time_ = 0.f;
}
float StabilityMonitor::stableTime() const { return stable_time_; }
