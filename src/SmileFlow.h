#pragma once
#include "ofMain.h"
#include <vector>
#include <string>
#include "StabilityMonitor.h"
#include "SmileEvaluator.h"

class SmileFlow {
public:
    enum Stage { STAGE_HOME = 0, STAGE_ALIGN, STAGE_HOLD_STILL, STAGE_PROMPT_SMILE, STAGE_EVALUATE };

    void reset();
    void setHoldStillSeconds(float s) { holdStillSeconds_ = s; }
    void setSmileHoldSeconds(float s) { smileHoldSeconds_ = s; }

    struct Inputs {
        bool   hasFace = false;
        bool   insideGuide = false;
        float  dt = 0.f;
        float  iod = 1.f;
        glm::vec2 faceCenter{0,0};
        glm::vec2 mouthLeft{0,0};
        glm::vec2 mouthRight{0,0};
        bool   haveMouth = false;
        std::vector<glm::vec2> derolledPoints;
    };

    void update(const Inputs& in);

    Stage stage() const { return stage_; }
    bool  abnormal() const { return abnormal_; }
    float stabilityProgress() const;
    float smileIntensity() const { return smile_.intensity(); }
    float smileAsymmetry() const { return smile_.asymmetry(); }
    std::vector<std::string> uiLines() const;

private:
    Stage stage_ = STAGE_HOME;
    StabilityMonitor stability_;
    SmileEvaluator   smile_;
    float  holdStillSeconds_ = 1.5f;
    float  smileHoldSeconds_ = 0.6f;
    float  smileHoldTime_ = 0.f;
    bool   abnormal_ = false;
};
