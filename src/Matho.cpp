// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 ifnoon
// Part of the ifnoon VCV Rack plugin project.

/*
 * Matho.cpp - Advanced Dual Slope Generator Module
 * 
 * This module provides advanced dual slope generation with chaos, breakpoints,
 * wavefolding, and derivative/integral outputs for complex signal processing.
 */

#include "plugin.hpp"
#include "CustomKnob.hpp"
#include "componentlibrary.hpp"

struct Matho : Module {
    enum ParamIds {
        // Slope A controls
        A_RISE_PARAM,
        A_FALL_PARAM,
        A_CURVE_PARAM,
        A_BREAKPOINT_PARAM,
        A_RATE_PARAM,
        
        // Slope B controls
        B_RISE_PARAM,
        B_FALL_PARAM,
        B_CURVE_PARAM,
        B_BREAKPOINT_PARAM,
        B_RATE_PARAM,
        
        // Global controls
        MIX_PARAM,
        CHAOS_A_PARAM,
        CHAOS_B_PARAM,
        FREEZE_A_BUTTON_PARAM,
        FREEZE_B_BUTTON_PARAM,
        PROBABILITY_PARAM,
        
        // Mode switches
        A_CYCLE_PARAM,
        B_CYCLE_PARAM,
        
        // Trigger mode switches
        A_TRIG_MODE_PARAM,
        B_TRIG_MODE_PARAM,
        
        NUM_PARAMS
    };
    
    enum InputIds {
        // Slope A inputs
        A_TRIG_INPUT,
        A_RISE_CV_INPUT,
        A_FALL_CV_INPUT,
        A_CURVE_CV_INPUT,
        A_BREAKPOINT_CV_INPUT,
        A_RATE_CV_INPUT,
        
        // Slope B inputs
        B_TRIG_INPUT,
        B_RISE_CV_INPUT,
        B_FALL_CV_INPUT,
        B_CURVE_CV_INPUT,
        B_BREAKPOINT_CV_INPUT,
        B_RATE_CV_INPUT,
        
        // Global inputs
        CHAOS_A_CV_INPUT,
        CHAOS_B_CV_INPUT,
        FREEZE_A_CV_INPUT,
        FREEZE_B_CV_INPUT,
        A_SYNC_INPUT,
        B_SYNC_INPUT,
        
        NUM_INPUTS
    };
    
    enum OutputIds {
        // Slope A outputs
        A_SLOPE_OUTPUT,
        A_END_OUTPUT,
        A_PULSE_OUTPUT,
        A_BREAKPOINT_OUTPUT,
        A_DERIVATIVE_OUTPUT,
        A_INTEGRAL_OUTPUT,
        
        // Slope B outputs
        B_SLOPE_OUTPUT,
        B_END_OUTPUT,
        B_PULSE_OUTPUT,
        B_BREAKPOINT_OUTPUT,
        B_DERIVATIVE_OUTPUT,
        B_INTEGRAL_OUTPUT,
        
        // Mixed outputs
        MIX_OUTPUT,
        
        // Mathematical operation outputs
        MIN_OUTPUT,
        MAX_OUTPUT,
        SUM_OUTPUT,
        
        NUM_OUTPUTS
    };
    
    enum LightIds {
        A_ACTIVE_LIGHT,
        A_END_LIGHT,
        A_BREAKPOINT_LIGHT,
        B_ACTIVE_LIGHT,
        B_END_LIGHT,
        B_BREAKPOINT_LIGHT,
        CHAOS_A_LIGHT,
        CHAOS_B_LIGHT,
        FREEZE_A_LIGHT,
        FREEZE_B_LIGHT,
        NUM_LIGHTS
    };

    // Slope state variables
    float aPhase = 0.0f;
    float bPhase = 0.0f;
    float aValue = 0.0f;
    float bValue = 0.0f;
    float aDerivative = 0.0f;
    float bDerivative = 0.0f;
    float aIntegral = 0.0f;
    float bIntegral = 0.0f;
    
    bool aRising = false;
    bool bRising = false;
    bool aEndPulse = false;
    bool bEndPulse = false;
    bool aBreakpointPulse = false;
    bool bBreakpointPulse = false;
    
    // Chaos and modulation
    float aChaosMod = 0.0f;
    float bChaosMod = 0.0f;
    bool freezeAState = false;
    bool freezeBState = false;
    
    // Previous trigger states for edge detection
    bool prevA_trigger = false;
    bool prevB_trigger = false;
    
    // Previous sync states for edge detection
    bool prevA_sync = false;
    bool prevB_sync = false;
    
    // Sync timing for PLL
    float lastSyncTimeA = 0.0f;
    float lastSyncTimeB = 0.0f;
    float syncPeriodA = 1.0f;  // Default 1 second period
    float syncPeriodB = 1.0f;  // Default 1 second period
    float currentTime = 0.0f;  // Accumulated time
    
    // Previous values for derivative calculation
    float prevAValue = 0.0f;
    float prevBValue = 0.0f;
    
    // Random number generator
    float randomValue = 0.0f;

    Matho() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Slope A parameters
        configParam(A_RISE_PARAM, 0.0f, 1.0f, 0.5f, "A Rise Time", "s");
        configParam(A_FALL_PARAM, 0.0f, 1.0f, 0.5f, "A Fall Time", "s");
        
        // Set custom display for time parameters - use simple multiplier for now
        getParamQuantity(A_RISE_PARAM)->displayMultiplier = 5.0f;
        getParamQuantity(A_RISE_PARAM)->displayOffset = 0.0f;
        getParamQuantity(A_FALL_PARAM)->displayMultiplier = 5.0f;
        getParamQuantity(A_FALL_PARAM)->displayOffset = 0.0f;
        configParam(A_CURVE_PARAM, -1.0f, 1.0f, 0.0f, "A Curvature");
        configParam(A_BREAKPOINT_PARAM, 0.0f, 1.0f, 0.5f, "A Breakpoint", "%");
        getParamQuantity(A_BREAKPOINT_PARAM)->displayMultiplier = 100.0f;
        getParamQuantity(A_BREAKPOINT_PARAM)->displayOffset = 0.0f;
        configParam(A_RATE_PARAM, 0.0f, 1.0f, 0.5f, "A Rate", "x");
        getParamQuantity(A_RATE_PARAM)->displayMultiplier = 2.0f;
        getParamQuantity(A_RATE_PARAM)->displayOffset = 0.0f;
        
        // Slope B parameters
        configParam(B_RISE_PARAM, 0.0f, 1.0f, 0.5f, "B Rise Time", "s");
        configParam(B_FALL_PARAM, 0.0f, 1.0f, 0.5f, "B Fall Time", "s");
        
        // Set custom display for time parameters - use simple multiplier for now
        getParamQuantity(B_RISE_PARAM)->displayMultiplier = 5.0f;
        getParamQuantity(B_RISE_PARAM)->displayOffset = 0.0f;
        getParamQuantity(B_FALL_PARAM)->displayMultiplier = 5.0f;
        getParamQuantity(B_FALL_PARAM)->displayOffset = 0.0f;
        configParam(B_CURVE_PARAM, -1.0f, 1.0f, 0.0f, "B Curvature");
        configParam(B_BREAKPOINT_PARAM, 0.0f, 1.0f, 0.5f, "B Breakpoint", "%");
        getParamQuantity(B_BREAKPOINT_PARAM)->displayMultiplier = 100.0f;
        getParamQuantity(B_BREAKPOINT_PARAM)->displayOffset = 0.0f;
        configParam(B_RATE_PARAM, 0.0f, 1.0f, 0.5f, "B Rate", "x");
        getParamQuantity(B_RATE_PARAM)->displayMultiplier = 2.0f;
        getParamQuantity(B_RATE_PARAM)->displayOffset = 0.0f;
        
        // Global parameters
        configParam(MIX_PARAM, 0.0f, 1.0f, 0.5f, "Mix", "%");
        configParam(CHAOS_A_PARAM, 0.0f, 1.0f, 0.0f, "A Chaos Amount");
        configParam(CHAOS_B_PARAM, 0.0f, 1.0f, 0.0f, "B Chaos Amount");
        configParam(FREEZE_A_BUTTON_PARAM, 0.0f, 1.0f, 0.0f, "A Freeze");
        configParam(FREEZE_B_BUTTON_PARAM, 0.0f, 1.0f, 0.0f, "B Freeze");
        configParam(PROBABILITY_PARAM, 0.0f, 1.0f, 1.0f, "End Probability", "%");
        
        // Mode switches
        configSwitch(A_CYCLE_PARAM, 0.0f, 1.0f, 0.0f, "A Cycle", {"Off", "On"});
        configSwitch(B_CYCLE_PARAM, 0.0f, 1.0f, 0.0f, "B Cycle", {"Off", "On"});
        
        // Trigger mode switches (4 positions)
        configSwitch(A_TRIG_MODE_PARAM, 0.0f, 3.0f, 0.0f, "A Trigger Mode", {"Always", "Rise Only", "Fall Only", "Complete Only"});
        configSwitch(B_TRIG_MODE_PARAM, 0.0f, 3.0f, 0.0f, "B Trigger Mode", {"Always", "Rise Only", "Fall Only", "Complete Only"});

        // Inputs
        configInput(A_TRIG_INPUT, "A Trigger");
        configInput(A_RISE_CV_INPUT, "A Rise CV");
        configInput(A_FALL_CV_INPUT, "A Fall CV");
        configInput(A_CURVE_CV_INPUT, "A Curve CV");
        configInput(A_BREAKPOINT_CV_INPUT, "A Breakpoint CV");
        configInput(A_RATE_CV_INPUT, "A Rate CV");
        
        configInput(B_TRIG_INPUT, "B Trigger");
        configInput(B_RISE_CV_INPUT, "B Rise CV");
        configInput(B_FALL_CV_INPUT, "B Fall CV");
        configInput(B_CURVE_CV_INPUT, "B Curve CV");
        configInput(B_BREAKPOINT_CV_INPUT, "B Breakpoint CV");
        configInput(B_RATE_CV_INPUT, "B Rate CV");
        
        configInput(CHAOS_A_CV_INPUT, "A Chaos CV");
        configInput(CHAOS_B_CV_INPUT, "B Chaos CV");
        configInput(FREEZE_A_CV_INPUT, "A Freeze CV");
        configInput(FREEZE_B_CV_INPUT, "B Freeze CV");
        configInput(A_SYNC_INPUT, "A Sync");
        configInput(B_SYNC_INPUT, "B Sync");
        
        // Outputs
        configOutput(A_SLOPE_OUTPUT, "A Slope");
        configOutput(A_END_OUTPUT, "A End");
        configOutput(A_PULSE_OUTPUT, "A Pulse");
        configOutput(A_BREAKPOINT_OUTPUT, "A Breakpoint");
        configOutput(A_DERIVATIVE_OUTPUT, "A Derivative");
        configOutput(A_INTEGRAL_OUTPUT, "A Integral");
        
        configOutput(B_SLOPE_OUTPUT, "B Slope");
        configOutput(B_END_OUTPUT, "B End");
        configOutput(B_PULSE_OUTPUT, "B Pulse");
        configOutput(B_BREAKPOINT_OUTPUT, "B Breakpoint");
        configOutput(B_DERIVATIVE_OUTPUT, "B Derivative");
        configOutput(B_INTEGRAL_OUTPUT, "B Integral");
        
        configOutput(MIX_OUTPUT, "Mix");
        configOutput(MIN_OUTPUT, "MIN");
        configOutput(MAX_OUTPUT, "MAX");
        configOutput(SUM_OUTPUT, "SUM");

        // Lights
        configLight(A_ACTIVE_LIGHT, "A Active");
        configLight(A_END_LIGHT, "A End");
        configLight(A_BREAKPOINT_LIGHT, "A Breakpoint");
        configLight(B_ACTIVE_LIGHT, "B Active");
        configLight(B_END_LIGHT, "B End");
        configLight(B_BREAKPOINT_LIGHT, "B Breakpoint");
        configLight(CHAOS_A_LIGHT, "A Chaos");
        configLight(CHAOS_B_LIGHT, "B Chaos");
        configLight(FREEZE_A_LIGHT, "A Freeze Active");
        configLight(FREEZE_B_LIGHT, "B Freeze Active");
        
        // Initialize chaos values with random values
        aChaosMod = (random::uniform() - 0.5f) * 2.0f; // -1.0 to +1.0
        bChaosMod = (random::uniform() - 0.5f) * 2.0f; // -1.0 to +1.0
    }

    void process(const ProcessArgs& args) override {
        float deltaTime = args.sampleTime;
        
        // Handle freeze A button (toggle on button press)
        bool freezeAButtonPressed = params[FREEZE_A_BUTTON_PARAM].getValue() > 0.5f;
        static bool prevFreezeAButton = false;
        if (freezeAButtonPressed && !prevFreezeAButton) {
            freezeAState = !freezeAState; // Toggle freeze A state
            // Debug: Print freeze state change
            printf("Freeze A toggled to: %s\n", freezeAState ? "ON" : "OFF");
        }
        prevFreezeAButton = freezeAButtonPressed;
        
        // Handle freeze A CV input (above 2V = freeze on, below = freeze off)
        // Only apply CV logic if there's a cable connected
        if (inputs[FREEZE_A_CV_INPUT].isConnected()) {
            float freezeACV = inputs[FREEZE_A_CV_INPUT].getVoltage();
            if (freezeACV > 2.0f) {
                freezeAState = true;
            } else if (freezeACV < 2.0f) {
                freezeAState = false;
            }
        }
        
        // Handle freeze B button (toggle on button press)
        bool freezeBButtonPressed = params[FREEZE_B_BUTTON_PARAM].getValue() > 0.5f;
        static bool prevFreezeBButton = false;
        if (freezeBButtonPressed && !prevFreezeBButton) {
            freezeBState = !freezeBState; // Toggle freeze B state
            // Debug: Print freeze state change
            printf("Freeze B toggled to: %s\n", freezeBState ? "ON" : "OFF");
        }
        prevFreezeBButton = freezeBButtonPressed;
        
        // Handle freeze B CV input (above 2V = freeze on, below = freeze off)
        // Only apply CV logic if there's a cable connected
        if (inputs[FREEZE_B_CV_INPUT].isConnected()) {
            float freezeBCV = inputs[FREEZE_B_CV_INPUT].getVoltage();
            if (freezeBCV > 2.0f) {
                freezeBState = true;
            } else if (freezeBCV < 2.0f) {
                freezeBState = false;
            }
        }
        
        lights[FREEZE_A_LIGHT].setBrightness(freezeAState ? 1.0f : 0.0f);
        lights[FREEZE_B_LIGHT].setBrightness(freezeBState ? 1.0f : 0.0f);
        
        // Get chaos amounts for each slope
        float chaosAAmount = params[CHAOS_A_PARAM].getValue() + inputs[CHAOS_A_CV_INPUT].getVoltage() * 0.1f;
        chaosAAmount = clamp(chaosAAmount, 0.0f, 1.0f);
        lights[CHAOS_A_LIGHT].setBrightness(chaosAAmount);
        
        float chaosBAmount = params[CHAOS_B_PARAM].getValue() + inputs[CHAOS_B_CV_INPUT].getVoltage() * 0.1f;
        chaosBAmount = clamp(chaosBAmount, 0.0f, 1.0f);
        lights[CHAOS_B_LIGHT].setBrightness(chaosBAmount);
        
        // Accumulate time for sync period measurement
        currentTime += deltaTime;
        
        // Process Slope A
        processSlope(A_TRIG_INPUT, A_RISE_PARAM, A_FALL_PARAM, A_CURVE_PARAM, 
                    A_BREAKPOINT_PARAM, A_RATE_PARAM, A_RISE_CV_INPUT, A_FALL_CV_INPUT, A_CURVE_CV_INPUT, 
                    A_BREAKPOINT_CV_INPUT, A_RATE_CV_INPUT, A_CYCLE_PARAM, A_SYNC_INPUT, A_TRIG_MODE_PARAM,
                    aPhase, aValue, aDerivative, aIntegral, aRising, aEndPulse, aBreakpointPulse,
                    A_ACTIVE_LIGHT, A_END_LIGHT, A_BREAKPOINT_LIGHT, deltaTime, freezeAState, chaosAAmount, prevA_trigger, aChaosMod, prevA_sync, lastSyncTimeA, syncPeriodA, currentTime);

        // Process Slope B
        processSlope(B_TRIG_INPUT, B_RISE_PARAM, B_FALL_PARAM, B_CURVE_PARAM, 
                    B_BREAKPOINT_PARAM, B_RATE_PARAM, B_RISE_CV_INPUT, B_FALL_CV_INPUT, B_CURVE_CV_INPUT, 
                    B_BREAKPOINT_CV_INPUT, B_RATE_CV_INPUT, B_CYCLE_PARAM, B_SYNC_INPUT, B_TRIG_MODE_PARAM,
                    bPhase, bValue, bDerivative, bIntegral, bRising, bEndPulse, bBreakpointPulse,
                    B_ACTIVE_LIGHT, B_END_LIGHT, B_BREAKPOINT_LIGHT, deltaTime, freezeBState, chaosBAmount, prevB_trigger, bChaosMod, prevB_sync, lastSyncTimeB, syncPeriodB, currentTime);
        
        // Set outputs
        outputs[A_SLOPE_OUTPUT].setVoltage(aValue * 10.0f);
        outputs[A_END_OUTPUT].setVoltage(aEndPulse ? 10.0f : 0.0f);
        outputs[A_PULSE_OUTPUT].setVoltage(aEndPulse ? 10.0f : 0.0f);
        outputs[A_BREAKPOINT_OUTPUT].setVoltage(aBreakpointPulse ? 10.0f : 0.0f);
        outputs[A_DERIVATIVE_OUTPUT].setVoltage(aDerivative * 5.0f);
        outputs[A_INTEGRAL_OUTPUT].setVoltage(aIntegral * 2.0f);
        
        outputs[B_SLOPE_OUTPUT].setVoltage(bValue * 10.0f);
        outputs[B_END_OUTPUT].setVoltage(bEndPulse ? 10.0f : 0.0f);
        outputs[B_PULSE_OUTPUT].setVoltage(bEndPulse ? 10.0f : 0.0f);
        outputs[B_BREAKPOINT_OUTPUT].setVoltage(bBreakpointPulse ? 10.0f : 0.0f);
        outputs[B_DERIVATIVE_OUTPUT].setVoltage(bDerivative * 5.0f);
        outputs[B_INTEGRAL_OUTPUT].setVoltage(bIntegral * 2.0f);
        
        // Mix outputs
        float mix = params[MIX_PARAM].getValue();
        float mixedOutput = aValue * (1.0f - mix) + bValue * mix;
        outputs[MIX_OUTPUT].setVoltage(mixedOutput * 10.0f);
        
        // Mathematical operation outputs (operate on both slopes)
        // MIN: minimum of both slope values
        outputs[MIN_OUTPUT].setVoltage(std::min(aValue, bValue) * 10.0f);
        
        // MAX: maximum of both slope values
        outputs[MAX_OUTPUT].setVoltage(std::max(aValue, bValue) * 10.0f);
        
        // SUM: sum of both slope values
        outputs[SUM_OUTPUT].setVoltage((aValue + bValue) * 10.0f);
        
        // Update previous values for derivative calculation
        prevAValue = aValue;
        prevBValue = bValue;
    }
    
private:
    void processSlope(int trigInput, int riseParam, int fallParam, int curveParam, 
                     int breakpointParam, int rateParam, int riseCvInput, int fallCvInput, int curveCvInput, 
                     int breakpointCvInput, int rateCvInput, int cycleParam, int syncInput, int trigModeParam,
                     float& phase, float& value, float& derivative, float& integral, 
                     bool& rising, bool& endPulse, bool& breakpointPulse,
                     int activeLight, int endLight, int breakpointLight,
                     float deltaTime, bool freeze, float chaosAmount, bool& prevTrigger, float& chaosMod, bool& prevSync, float& lastSyncTime, float& syncPeriod, float currentTime) {
        
        // Check for trigger (rising edge detection) with mode-based acceptance
        bool currentTrigger = inputs[trigInput].getVoltage() > 0.5f;
        bool triggerAccepted = false;
        
        // Reset prevTrigger state if input is disconnected to prevent stuck state
        if (!inputs[trigInput].isConnected()) {
            prevTrigger = false;
            // Reset ALL envelope state variables to neutral state
            phase = 0.0f;
            rising = true;
            value = 0.0f;
            derivative = 0.0f;
            integral = 0.0f;
            endPulse = false;
            breakpointPulse = false;
        }
        
        if (currentTrigger && !prevTrigger) {
            int trigMode = (int)params[trigModeParam].getValue();
            
            // Check if trigger should be accepted based on current state
            switch (trigMode) {
                case 0: // Always accept trigger
                    triggerAccepted = true;
                    break;
                case 1: // Only during rise stage
                    triggerAccepted = rising && phase < 1.0f;
                    break;
                case 2: // Only during fall stage
                    triggerAccepted = !rising && phase < 1.0f;
                    break;
                case 3: // Only when slope is completed (at end of fall)
                    triggerAccepted = !rising && phase >= 1.0f;
                    break;
            }
            
            if (triggerAccepted) {
                phase = 0.0f;
                rising = true;
                value = 0.0f;
                
                // Generate new random chaos modulation for this cycle
                chaosMod = (random::uniform() - 0.5f) * 2.0f; // -1.0 to +1.0
            }
        }
        prevTrigger = currentTrigger;
        
        // Check for sync (rising edge detection) and implement PLL
        // Auto-detect sync: enabled when cable is connected (input is not floating)
        bool syncEnabled = inputs[syncInput].isConnected();
        bool currentSync = inputs[syncInput].getVoltage() > 0.5f;
        
        // Reset prevSync state if input is disconnected to prevent stuck state
        if (!inputs[syncInput].isConnected()) {
            prevSync = false;
        }
        
        if (syncEnabled && currentSync && !prevSync) {
            // Measure time since last sync pulse to determine clock period
            if (lastSyncTime > 0.0f) {
                syncPeriod = currentTime - lastSyncTime;
                // Clamp sync period to reasonable range (10ms to 10 seconds)
                syncPeriod = clamp(syncPeriod, 0.01f, 10.0f);
            }
            lastSyncTime = currentTime;
            
            // Reset envelope to start of cycle when sync pulse received
            phase = 0.0f;
            rising = true;
            value = 0.0f;
            
            // Generate new random chaos modulation for this cycle
            chaosMod = (random::uniform() - 0.5f) * 2.0f; // -1.0 to +1.0
        }
        prevSync = currentSync;
        
        
        if (freeze) return;
        
        // Get parameters with CV and apply non-linear mapping
        // First half of knob (0-0.5): 0 to 1 second (exponential curve)
        // Second half of knob (0.5-1.0): 1 to 5 seconds (exponential curve)
        float riseParamValue = params[riseParam].getValue() + inputs[riseCvInput].getVoltage() * 0.1f;
        float fallParamValue = params[fallParam].getValue() + inputs[fallCvInput].getVoltage() * 0.1f;
        
        // Clamp to 0-1 range since parameters are now 0-1
        riseParamValue = clamp(riseParamValue, 0.0f, 1.0f);
        fallParamValue = clamp(fallParamValue, 0.0f, 1.0f);
        
        float riseTime = (riseParamValue <= 0.5f) ? 
            riseParamValue * 2.0f :  // 0-0.5 maps to 0-1 second (linear for precise control)
            1.0f + pow((riseParamValue - 0.5f) * 2.0f, 2.0f) * 4.0f; // 0.5-1.0 maps to 1-5 seconds (exponential)
            
        float fallTime = (fallParamValue <= 0.5f) ? 
            fallParamValue * 2.0f :  // 0-0.5 maps to 0-1 second (linear for precise control)
            1.0f + pow((fallParamValue - 0.5f) * 2.0f, 2.0f) * 4.0f; // 0.5-1.0 maps to 1-5 seconds (exponential)
        float curve = params[curveParam].getValue() + inputs[curveCvInput].getVoltage() * 0.1f;
        float breakpoint = params[breakpointParam].getValue() + inputs[breakpointCvInput].getVoltage() * 0.1f;
        breakpoint = clamp(breakpoint, 0.0f, 0.9999f); // Cap at 99.99% to ensure trigger
        
        curve = clamp(curve, -1.0f, 1.0f);
        breakpoint = clamp(breakpoint, 0.0f, 1.0f);
        
        // Apply chaos modulation (sample and hold per cycle)
        if (chaosAmount > 0.0f) {
            float chaosEffect = chaosMod * chaosAmount;
            riseTime *= (1.0f + chaosEffect * 1.0f); // Reduced back to 1.0f to prevent extreme values
            fallTime *= (1.0f + chaosEffect * 1.0f);
            
            // Apply chaos to curve as well (moderate effect to keep it musical)
            curve += chaosEffect * 0.5f; // 0.5f scaling for moderate curve variation
            curve = clamp(curve, -1.0f, 1.0f); // Keep curve in valid range
        }
        
        // Apply rate control (adds/subtracts CV to both rise and fall times)
        float rateValue = params[rateParam].getValue(); // 0.0 to 1.0 (noon = 0.5)
        float rateCv = inputs[rateCvInput].getVoltage(); // -5V to +5V
        float rateOffset = (rateValue - 0.5f) * 2.0f + rateCv * 0.2f; // Convert knob to -1 to +1 range, CV scaled to -1 to +1
        
        riseTime += rateOffset;
        fallTime += rateOffset;
        
        // Final bounds checking for timing values
        riseTime = clamp(riseTime, 0.001f, 20.0f);
        fallTime = clamp(fallTime, 0.001f, 20.0f);
        
        // Update phase
        float envelopeTime = rising ? riseTime : fallTime;
        
        // Apply sync period scaling when sync is enabled
        if (syncEnabled && syncPeriod > 0.0f) {
            // Scale envelope time to match the sync period
            // The envelope should complete one full cycle (rise + fall) in syncPeriod time
            float totalEnvelopeTime = riseTime + fallTime;
            float syncScale = syncPeriod / totalEnvelopeTime;
            envelopeTime *= syncScale;
        }
        
        phase += deltaTime / envelopeTime;
        
        // Check for breakpoint (as percentage of entire envelope cycle)
        bool breakpointTriggered = false;
        float cyclePhase = rising ? phase : (1.0f + phase); // 0-1 for rise, 1-2 for fall
        float breakpointPhase = breakpoint * 2.0f; // Convert 0-1 breakpoint to 0-2 cycle phase
        
        if (cyclePhase >= breakpointPhase && !breakpointPulse) {
            breakpointTriggered = true;
            breakpointPulse = true;
        }
        
        // Check for end of cycle
        if (phase >= 1.0f) {
            if (rising) {
                rising = false;
                phase = 0.0f;
                value = 1.0f;
            } else {
                // Only restart cycle if sync is not enabled (sync takes priority)
                if (params[cycleParam].getValue() > 0.5f && !syncEnabled) {
                    rising = true;
                    phase = 0.0f;
                    value = 0.0f;
                    
                    // Generate new random chaos modulation for this cycle
                    chaosMod = (random::uniform() - 0.5f) * 2.0f; // -1.0 to +1.0
                } else {
                    phase = 1.0f;
                    value = 0.0f;
                }
                endPulse = true;
                // Only reset breakpoint pulse when envelope actually completes
                breakpointPulse = false;
            }
        }
        
        // Calculate slope value with curve and skew
        float rawPhase = phase;
        if (curve != 0.0f) {
            rawPhase = applyCurve(phase, curve);
        }
        
        value = rising ? rawPhase : (1.0f - rawPhase);
        
        // Calculate derivative
        derivative = (value - (rising ? (rawPhase - deltaTime/currentTime) : (1.0f - (rawPhase - deltaTime/currentTime)))) / deltaTime;
        
        // Update integral
        integral += value * deltaTime;
        
        // Set lights
        lights[activeLight].setBrightness(value);
        lights[endLight].setBrightness(endPulse ? 1.0f : 0.0f);
        lights[breakpointLight].setBrightness(breakpointTriggered ? 1.0f : 0.0f);
        
        // Reset pulses
        endPulse = false;
    }
    
    float applyCurve(float phase, float curve) {
        // Enhanced curve implementation with more snappy exponential
        // curve: -1.0 = logarithmic, 0.0 = linear, +1.0 = very snappy exponential
        
        if (curve < 0.0f) {
            // Logarithmic curve (counterclockwise): slow start, snappy end
            float shape = -curve; // 0 to 1
            return pow(phase, 1.0f + shape * 2.0f); // 1.0 to 3.0
        } else if (curve > 0.0f) {
            // Exponential curve (clockwise): very snappy start, slow end  
            float shape = curve; // 0 to 1
            return 1.0f - pow(1.0f - phase, 1.0f + shape * 8.0f); // 1.0 to 9.0 (much snappier)
        } else {
            // Linear curve (center)
            return phase;
        }
    }
    
    
};

struct MathoWidget : ModuleWidget {
    MathoWidget(Matho* module) {
        setModule(module);

        // 20HP panel
        box.size = Vec(RACK_GRID_WIDTH * 20, RACK_GRID_HEIGHT);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Matho.svg")));

        // Using VCV Rack standard mm2px function

        // === SLOPE A CONTROLS (Left side) ===
        // A Trigger (top-left starting point)
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8, 10)), module, Matho::A_TRIG_INPUT));
        
        // A Rise/Fall knobs
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8, 20)), module, Matho::A_RISE_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8, 30)), module, Matho::A_RISE_CV_INPUT));
        
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8, 40)), module, Matho::A_FALL_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8, 50)), module, Matho::A_FALL_CV_INPUT));
        
        // A Curve/Breakpoint
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8, 60)), module, Matho::A_CURVE_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8, 70)), module, Matho::A_CURVE_CV_INPUT));
        
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8, 80)), module, Matho::A_BREAKPOINT_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8, 90)), module, Matho::A_BREAKPOINT_CV_INPUT));
        
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8, 100)), module, Matho::A_RATE_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8, 110)), module, Matho::A_RATE_CV_INPUT));
        
        // A Trigger Mode switch
        addParam(createParamCentered<CKSS>(mm2px(Vec(18, 100)), module, Matho::A_TRIG_MODE_PARAM));
        
        // A Cycle switch
        addParam(createParamCentered<CKSS>(mm2px(Vec(18, 110)), module, Matho::A_CYCLE_PARAM));
        
        // A Outputs
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8, 120)), module, Matho::A_SLOPE_OUTPUT));
        
        // A Lights
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(18, 10)), module, Matho::A_ACTIVE_LIGHT));
        addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(18, 110)), module, Matho::A_END_LIGHT));
        addChild(createLightCentered<SmallLight<BlueLight>>(mm2px(Vec(18, 90)), module, Matho::A_BREAKPOINT_LIGHT));

        // === SLOPE B CONTROLS (Right side) ===
        // B Trigger
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(92, 10)), module, Matho::B_TRIG_INPUT));
        
        // B Rise/Fall knobs
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(92, 20)), module, Matho::B_RISE_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(92, 30)), module, Matho::B_RISE_CV_INPUT));
        
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(92, 40)), module, Matho::B_FALL_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(92, 50)), module, Matho::B_FALL_CV_INPUT));
        
        // B Curve/Breakpoint
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(92, 60)), module, Matho::B_CURVE_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(92, 70)), module, Matho::B_CURVE_CV_INPUT));
        
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(92, 80)), module, Matho::B_BREAKPOINT_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(92, 90)), module, Matho::B_BREAKPOINT_CV_INPUT));
        
        // B Trigger Mode switch
        addParam(createParamCentered<CKSS>(mm2px(Vec(82, 100)), module, Matho::B_TRIG_MODE_PARAM));
        
        // B Cycle switch
        addParam(createParamCentered<CKSS>(mm2px(Vec(82, 110)), module, Matho::B_CYCLE_PARAM));
        
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(92, 100)), module, Matho::B_RATE_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(92, 110)), module, Matho::B_RATE_CV_INPUT));
        
        // B Outputs
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(92, 120)), module, Matho::B_SLOPE_OUTPUT));
        
        // B Lights
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(82, 10)), module, Matho::B_ACTIVE_LIGHT));
        addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(82, 110)), module, Matho::B_END_LIGHT));
        addChild(createLightCentered<SmallLight<BlueLight>>(mm2px(Vec(82, 90)), module, Matho::B_BREAKPOINT_LIGHT));

        // === CENTER CONTROLS ===
        // Mix knob
        addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(50, 20)), module, Matho::MIX_PARAM));
        
        // Chaos A and B
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(40, 40)), module, Matho::CHAOS_A_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(40, 50)), module, Matho::CHAOS_A_CV_INPUT));
        
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60, 40)), module, Matho::CHAOS_B_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(60, 50)), module, Matho::CHAOS_B_CV_INPUT));
        
        // Probability
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(50, 60)), module, Matho::PROBABILITY_PARAM));
        
        // Freeze A Button
        addParam(createParamCentered<LEDButton>(mm2px(Vec(40, 70)), module, Matho::FREEZE_A_BUTTON_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(40, 80)), module, Matho::FREEZE_A_CV_INPUT));
        
        // Freeze B Button
        addParam(createParamCentered<LEDButton>(mm2px(Vec(60, 70)), module, Matho::FREEZE_B_BUTTON_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(60, 80)), module, Matho::FREEZE_B_CV_INPUT));
        
        // A Sync (auto-detect when cable connected)
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30, 30)), module, Matho::A_SYNC_INPUT));
        
        // B Sync (auto-detect when cable connected)
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(70, 30)), module, Matho::B_SYNC_INPUT));
        
        // Mixed Outputs
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(45, 90)), module, Matho::MIX_OUTPUT));
        
        // Mathematical Operation Outputs
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35, 100)), module, Matho::MIN_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(45, 100)), module, Matho::MAX_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(55, 100)), module, Matho::SUM_OUTPUT));
        
        // === CENTER LIGHTS ===
        // Chaos Lights
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(40, 60)), module, Matho::CHAOS_A_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(60, 60)), module, Matho::CHAOS_B_LIGHT));
        
        // Freeze Lights
        addChild(createLightCentered<SmallLight<BlueLight>>(mm2px(Vec(40, 90)), module, Matho::FREEZE_A_LIGHT));
        addChild(createLightCentered<SmallLight<BlueLight>>(mm2px(Vec(60, 90)), module, Matho::FREEZE_B_LIGHT));

        // Add VCV Rack mounting screws
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    }
};

Model* modelMatho = createModel<Matho, MathoWidget>("Matho");