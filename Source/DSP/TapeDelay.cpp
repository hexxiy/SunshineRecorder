#include "TapeDelay.h"
#include <algorithm>

namespace palace {

void TapeDelay::prepare(double sampleRate, int /*maxBlockSize*/) {
    sampleRate_ = sampleRate;

    // Max 2000ms + 5% flutter headroom
    maxBufferSize_ = static_cast<int>(sampleRate * 2.1) + 4; // +4 for Hermite interpolation
    bufferL_.resize(static_cast<size_t>(maxBufferSize_), 0.0f);
    bufferR_.resize(static_cast<size_t>(maxBufferSize_), 0.0f);

    targetDelaySamples_ = static_cast<float>(delayTimeMs_ * 0.001 * sampleRate_);
    smoothedDelaySamples_ = targetDelaySamples_;

    writePos_ = 0;
}

void TapeDelay::reset() {
    std::fill(bufferL_.begin(), bufferL_.end(), 0.0f);
    std::fill(bufferR_.begin(), bufferR_.end(), 0.0f);
    writePos_ = 0;
    lfoPhase1_ = 0.0f;
    lfoPhase2_ = 0.0f;
    dcPrevInL_ = 0.0f;
    dcPrevOutL_ = 0.0f;
    dcPrevInR_ = 0.0f;
    dcPrevOutR_ = 0.0f;
    smoothedDelaySamples_ = targetDelaySamples_;
}

void TapeDelay::setDelayTime(float delayMs) {
    delayTimeMs_ = delayMs;
    targetDelaySamples_ = static_cast<float>(delayMs * 0.001 * sampleRate_);
}

void TapeDelay::setFeedback(float fb) {
    feedback_ = fb;
}

void TapeDelay::setFlutter(float amount) {
    flutterAmount_ = amount;
}

void TapeDelay::setHiss(float amount) {
    hissAmount_ = amount;
}

float TapeDelay::hermiteInterpolate(const std::vector<float>& buffer, float position) const {
    int size = static_cast<int>(buffer.size());

    int i0 = static_cast<int>(position);
    float frac = position - static_cast<float>(i0);

    // Wrap indices
    int im1 = ((i0 - 1) % size + size) % size;
    int i1  = (i0 + 1) % size;
    int i2  = (i0 + 2) % size;
    i0 = i0 % size;

    float y0 = buffer[static_cast<size_t>(im1)];
    float y1 = buffer[static_cast<size_t>(i0)];
    float y2 = buffer[static_cast<size_t>(i1)];
    float y3 = buffer[static_cast<size_t>(i2)];

    // Hermite interpolation coefficients
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

void TapeDelay::process(float* leftChannel, float* rightChannel, int numSamples) {
    if (maxBufferSize_ == 0) return;

    const float lfoInc1 = static_cast<float>(lfoFreq1_ / sampleRate_);
    const float lfoInc2 = static_cast<float>(lfoFreq2_ / sampleRate_);
    const float twoPi = 6.283185307f;

    for (int i = 0; i < numSamples; ++i) {
        // 1. Smooth delay time toward target
        smoothedDelaySamples_ += smoothingCoeff_ * (targetDelaySamples_ - smoothedDelaySamples_);

        // 2. Compute flutter offset from two sine LFOs
        float lfo1 = std::sin(lfoPhase1_ * twoPi);
        float lfo2 = std::sin(lfoPhase2_ * twoPi);
        // At 100% flutter, wobble is ±4% of delay time
        float flutterOffset = flutterAmount_ * 0.04f * smoothedDelaySamples_ * (lfo1 * 0.6f + lfo2 * 0.4f);

        // Advance LFO phases
        lfoPhase1_ += lfoInc1;
        if (lfoPhase1_ >= 1.0f) lfoPhase1_ -= 1.0f;
        lfoPhase2_ += lfoInc2;
        if (lfoPhase2_ >= 1.0f) lfoPhase2_ -= 1.0f;

        // 3. Read from delay buffer using Hermite interpolation
        float readPos = static_cast<float>(writePos_) - smoothedDelaySamples_ - flutterOffset;
        // Wrap into positive range
        while (readPos < 0.0f) readPos += static_cast<float>(maxBufferSize_);

        float wetL = hermiteInterpolate(bufferL_, readPos);
        float wetR = hermiteInterpolate(bufferR_, readPos);

        // 4. Add hiss noise to wet signal
        if (hissAmount_ > 0.0f) {
            wetL += noiseDist_(rng_) * hissAmount_ * 0.03f;
            wetR += noiseDist_(rng_) * hissAmount_ * 0.03f;
        }

        // 5. Soft-clip (cubic saturation) and DC-block the feedback path
        float fbL = wetL * feedback_;
        float fbR = wetR * feedback_;

        // Cubic soft clipper: x - x^3/3
        fbL = fbL - (fbL * fbL * fbL) / 3.0f;
        fbR = fbR - (fbR * fbR * fbR) / 3.0f;

        // DC blocking filter: y[n] = x[n] - x[n-1] + coeff * y[n-1]
        float dcOutL = fbL - dcPrevInL_ + dcCoeff_ * dcPrevOutL_;
        dcPrevInL_ = fbL;
        dcPrevOutL_ = dcOutL;
        fbL = dcOutL;

        float dcOutR = fbR - dcPrevInR_ + dcCoeff_ * dcPrevOutR_;
        dcPrevInR_ = fbR;
        dcPrevOutR_ = dcOutR;
        fbR = dcOutR;

        // 6. Write input + feedback to delay buffer
        bufferL_[static_cast<size_t>(writePos_)] = leftChannel[i] + fbL;
        bufferR_[static_cast<size_t>(writePos_)] = rightChannel[i] + fbR;

        // 7. Add wet signal to output
        leftChannel[i] += wetL;
        rightChannel[i] += wetR;

        // Advance write position
        writePos_ = (writePos_ + 1) % maxBufferSize_;
    }
}

} // namespace palace
