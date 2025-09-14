#include "SmileEvaluator.h"
#include "ofMain.h"

void SmileEvaluator::reset() {
    calibrated_ = false;
    base_left_ = {0,0};
    base_right_ = {0,0};
    base_iod_ = 1.f;
    smile_intensity_ = 0.f;
    asymmetry_score_ = 0.f;
}
void SmileEvaluator::calibrate(const glm::vec2& left_corner, const glm::vec2& right_corner, float iod) {
    base_left_ = left_corner;
    base_right_ = right_corner;
    base_iod_ = iod;
    calibrated_ = true;
}
void SmileEvaluator::updateMetrics(const std::vector<glm::vec2>& ptsDerolled, float iod) {
    if (!calibrated_ || ptsDerolled.size() <= LM_RIGHT_MOUTH_CORNER) {
        decayToZero(0.2f);
        return;
    }
    glm::vec2 L = ptsDerolled[LM_LEFT_MOUTH_CORNER];
    glm::vec2 R = ptsDerolled[LM_RIGHT_MOUTH_CORNER];
    float leftRaise  = (base_left_.y  - L.y) / std::max(1.f, iod);
    float rightRaise = (base_right_.y - R.y) / std::max(1.f, iod);
    float mean_abs = 0.5f * (std::abs(leftRaise) + std::abs(rightRaise));
    float intensity = std::max(0.0f, mean_abs);
    float denom = mean_abs + 1e-6f;
    float asym  = std::abs(leftRaise - rightRaise) / denom;
    smile_intensity_ = (1.f - smoothing_alpha) * smile_intensity_ + smoothing_alpha * intensity;
    asymmetry_score_ = (1.f - smoothing_alpha) * asymmetry_score_ + smoothing_alpha * asym;
}
void SmileEvaluator::decayToZero(float alpha) {
    smile_intensity_ = ofLerp(smile_intensity_, 0.0f, alpha);
    asymmetry_score_ = ofLerp(asymmetry_score_, 0.0f, alpha);
}
bool SmileEvaluator::isCalibrated() const { return calibrated_; }
float SmileEvaluator::baseIOD() const { return base_iod_; }
float SmileEvaluator::intensity() const { return smile_intensity_; }
float SmileEvaluator::asymmetry() const { return asymmetry_score_; }
bool  SmileEvaluator::aboveAsymmetryThreshold() const { return asymmetry_score_ > asym_threshold; }
