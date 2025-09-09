#pragma once
#include "ofMain.h"
#include "ofxFaceTracker2.h"

// Small helper for rotating a 2D point around a center (used to de-roll the face)
static inline glm::vec2 rotateAround(const glm::vec2& p, const glm::vec2& c, float rad) {
    glm::vec2 t = p - c;
    float cs = std::cos(rad), sn = std::sin(rad);
    return glm::vec2(cs * t.x - sn * t.y, sn * t.x + cs * t.y) + c;
}

class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void keyPressed(int key) override;

    // Camera + tracker
    ofVideoGrabber  grabber;
    ofxFaceTracker2 tracker;

private:
    // --- UI / view ---
    bool mirrorView = true;    // mirror like a selfie to reduce user confusion

    // --- Guide oval (where the head should be) ---
    // Computed in setup() from camera size.
    glm::vec2 guideCenter = {0,0};
    float guideA = 100.f; // ellipse semi-axis X (pixels)
    float guideB = 140.f; // ellipse semi-axis Y (pixels)

    // --- Simple state machine for the flow ---
    enum Stage { STAGE_ALIGN = 0, STAGE_HOLD_STILL, STAGE_PROMPT_SMILE, STAGE_EVALUATE };
    Stage stage = STAGE_ALIGN;

    // --- Stability / calibration ---
    glm::vec2 prevCenter = {0,0};
    bool havePrevCenter = false;
    float stableTime = 0.f;              // seconds of continuous stability
    const float HOLD_STILL_SECONDS = 1.5f;
    const float MOVE_THRESH_NORM = 0.012f; // normalized by inter-ocular distance (IOD)

    // --- Baseline (neutral) mouth corners + size normalization ---
    glm::vec2 baseLeftCorner  = {0,0};   // landmark 48 (derolled)
    glm::vec2 baseRightCorner = {0,0};   // landmark 54 (derolled)
    float baseIOD = 1.f;                 // inter-ocular distance at neutral (derolled)
    bool calibrated = false;

    // --- Live metrics (smile + asymmetry), smoothed ---
    float smileIntensity = 0.f;          // normalized [0..~1]
    float asymmetryScore = 0.f;          // 0 = perfectly symmetric
    float smoothingAlpha = 0.2f;         // EMA factor
    const float SMILE_MIN = 0.06f;       // need a real smile to evaluate
    const float ASYM_THRESHOLD = 0.35f;  // >35% imbalance → flag

    // When user actually smiles (above threshold) for a short moment
    float smileHoldTime = 0.f;
    const float SMILE_HOLD_SECONDS = 0.6f;

    // Final result after evaluation
    bool abnormal = false;

    // --- Landmark constants (dlib 68) ---
    static constexpr int LM_LEFT_EYE_START  = 36;
    static constexpr int LM_LEFT_EYE_END    = 41;
    static constexpr int LM_RIGHT_EYE_START = 42;
    static constexpr int LM_RIGHT_EYE_END   = 47;
    static constexpr int LM_LEFT_MOUTH_CORNER  = 48;
    static constexpr int LM_RIGHT_MOUTH_CORNER = 54;

    // --- Helpers ---
    bool hasFace() const;
    bool getDerolledKeypoints(std::vector<glm::vec2>& outPts,
                              glm::vec2& outLeftEyeC,
                              glm::vec2& outRightEyeC,
                              glm::vec2& outFaceCenter) const;
    float interOcular(const glm::vec2& L, const glm::vec2& R) const {
        return std::max(1.0f, glm::distance(L, R));
    }
    bool mostlyInsideGuide(const std::vector<glm::vec2>& pts, float fractionNeeded = 0.92f) const;
    void updateStability(const glm::vec2& center, float iod, bool insideGuide, float dt);
    void updateSmileMetrics(const std::vector<glm::vec2>& ptsDerolled, float iod);
    void resetAll();
};
