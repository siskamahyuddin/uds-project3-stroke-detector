#pragma once
#include "ofMain.h"
#include <vector>
#include "Landmarks68.h"

class SmileEvaluator {
public:
    float smoothing_alpha = 0.2f;
    float smile_min       = 0.06f;
    float asym_threshold  = 0.35f;

    void reset();
    void calibrate(const glm::vec2& left_corner, const glm::vec2& right_corner, float iod);
    void updateMetrics(const std::vector<glm::vec2>& ptsDerolled, float iod);
    void decayToZero(float alpha);

    bool  isCalibrated() const;
    float baseIOD() const;
    float intensity() const;
    float asymmetry() const;
    bool  aboveAsymmetryThreshold() const;

private:
    glm::vec2 base_left_{0,0};
    glm::vec2 base_right_{0,0};
    float     base_iod_ = 1.f;
    bool      calibrated_ = false;

    float smile_intensity_ = 0.f;
    float asymmetry_score_ = 0.f;
};
