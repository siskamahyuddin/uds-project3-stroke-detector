#include "SmileFlow.h"
#include "ofMain.h"

void SmileFlow::reset() {
    stage_ = STAGE_ALIGN;
    stability_.reset();
    smile_.reset();
    smileHoldTime_ = 0.f;
    abnormal_ = false;
}

void SmileFlow::update(const Inputs& in) {
    if (!in.hasFace) {
        smile_.decayToZero(0.08f);
        if (stage_ != STAGE_EVALUATE) stage_ = STAGE_ALIGN;
        return;
    }
    switch (stage_) {
        case STAGE_ALIGN: {
            stability_.reset();
            smileHoldTime_ = 0.f;
            if (in.insideGuide) stage_ = STAGE_HOLD_STILL;
            break;
        }
        case STAGE_HOLD_STILL: {
            stability_.update(in.faceCenter, in.iod, in.insideGuide, in.dt);
            if (!in.insideGuide) { stage_ = STAGE_ALIGN; break; }
            if (stability_.stableTime() >= holdStillSeconds_) {
                if (in.haveMouth) smile_.calibrate(in.mouthLeft, in.mouthRight, in.iod);
                stage_ = STAGE_PROMPT_SMILE;
            }
            break;
        }
        case STAGE_PROMPT_SMILE: {
            if (!in.insideGuide) { stage_ = STAGE_ALIGN; break; }
            if (!in.derolledPoints.empty()) {
                smile_.updateMetrics(in.derolledPoints, in.iod);
            } else if (in.haveMouth) {
                std::vector<glm::vec2> two = { in.mouthLeft, in.mouthRight };
                smile_.updateMetrics(two, in.iod);
            }
            if (smile_.intensity() >= smile_.smile_min) smileHoldTime_ += in.dt;
            else smileHoldTime_ = 0.f;
            if (smileHoldTime_ >= smileHoldSeconds_) {
                abnormal_ = smile_.aboveAsymmetryThreshold();
                stage_ = STAGE_EVALUATE;
            }
            break;
        }
        case STAGE_EVALUATE: {
            break;
        }
        default: break;
    }
}

float SmileFlow::stabilityProgress() const {
    float p = stability_.stableTime() / std::max(0.0001f, holdStillSeconds_);
    return ofClamp(p, 0.f, 1.f);
}

std::vector<std::string> SmileFlow::uiLines() const {
    std::vector<std::string> lines;
    switch (stage_) {
        case STAGE_ALIGN:
            lines.push_back("Align your head inside the oval.");
            lines.push_back("Keep your head roughly upright.");
            break;
        case STAGE_HOLD_STILL:
            lines.push_back("Hold still...");
            lines.push_back("Capturing neutral baseline.");
            lines.push_back("Progress: " + ofToString(stabilityProgress() * 100.f, 0) + "%");
            break;
        case STAGE_PROMPT_SMILE:
            lines.push_back("Show your teeth (smile)!");
            lines.push_back("Hold for a moment...");
            lines.push_back("Smile intensity: " + ofToString(smile_.intensity(), 3) + "  |  Asym: " + ofToString(smile_.asymmetry(), 3));
            break;
        case STAGE_EVALUATE:
            lines.push_back("Result:");
            lines.push_back("Press [R] to restart");
            break;
        default: break;
    }
    return lines;
}
