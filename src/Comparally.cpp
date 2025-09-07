// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 ifnoon
// Part of the ifnoon VCV Rack plugin project.

/*
 * Comparally.cpp - Advanced 4-Channel Window Comparator Module
 * 
 * This module provides a sophisticated 4-channel window comparator with logic outputs
 * for complex signal analysis and conditional routing in VCV Rack.
 * 
 * Features:
 * - 4 independent comparator channels with adjustable thresholds
 * - Window comparator mode (upper/lower bounds)
 * - Logic outputs for AB, CD, and Pairs comparisons
 * - Visual feedback with status LEDs
 * - High-precision voltage comparison
 */

#include "plugin.hpp"
#include "CustomKnob.hpp"
#include "componentlibrary.hpp"
#include <algorithm>

struct Comparally : Module {
    enum ParamIds {
        A_SHIFT_PARAM,
        A_SIZE_PARAM,
        B_SHIFT_PARAM,
        B_SIZE_PARAM,
        C_SHIFT_PARAM,
        C_SIZE_PARAM,
        D_SHIFT_PARAM,
        D_SIZE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        A_IN_INPUT,
        A_SHIFT_CV_INPUT,
        A_SIZE_CV_INPUT,
        B_IN_INPUT,
        B_SHIFT_CV_INPUT,
        B_SIZE_CV_INPUT,
        C_IN_INPUT,
        C_SHIFT_CV_INPUT,
        C_SIZE_CV_INPUT,
        D_IN_INPUT,
        D_SHIFT_CV_INPUT,
        D_SIZE_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        A_HI_OUTPUT,
        A_WIN_OUTPUT,
        A_LO_OUTPUT,
        B_HI_OUTPUT,
        B_WIN_OUTPUT,
        B_LO_OUTPUT,
        C_HI_OUTPUT,
        C_WIN_OUTPUT,
        C_LO_OUTPUT,
        D_HI_OUTPUT,
        D_WIN_OUTPUT,
        D_LO_OUTPUT,
        // Pair A+B logic outputs
        AB_AND_OUTPUT,
        AB_OR_OUTPUT,
        AB_XOR_OUTPUT,
        AB_FF_OUTPUT,
        // Pair C+D logic outputs
        CD_AND_OUTPUT,
        CD_OR_OUTPUT,
        CD_XOR_OUTPUT,
        CD_FF_OUTPUT,
        // Combined pairs logic outputs
        PAIRS_AND_OUTPUT,
        PAIRS_OR_OUTPUT,
        PAIRS_XOR_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        A_HI_LIGHT,
        A_WIN_LIGHT,
        A_LO_LIGHT,
        B_HI_LIGHT,
        B_WIN_LIGHT,
        B_LO_LIGHT,
        C_HI_LIGHT,
        C_WIN_LIGHT,
        C_LO_LIGHT,
        D_HI_LIGHT,
        D_WIN_LIGHT,
        D_LO_LIGHT,
        // Pair A+B logic lights
        AB_AND_LIGHT,
        AB_OR_LIGHT,
        AB_XOR_LIGHT,
        AB_FF_LIGHT,
        // Pair C+D logic lights
        CD_AND_LIGHT,
        CD_OR_LIGHT,
        CD_XOR_LIGHT,
        CD_FF_LIGHT,
        // Combined pairs logic lights
        PAIRS_AND_LIGHT,
        PAIRS_OR_LIGHT,
        PAIRS_XOR_LIGHT,
        NUM_LIGHTS
    };

    // per-channel state
    bool aHi = false, aWin = false, aLo = false;
    bool bHi = false, bWin = false, bLo = false;
    bool cHi = false, cWin = false, cLo = false;
    bool dHi = false, dWin = false, dLo = false;

    // pair logic state
    bool abFlipFlop = false;
    bool cdFlipFlop = false;
    bool abXorPrevious = false;
    bool cdXorPrevious = false;

    Comparally() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // SHIFT knobs (±5 V)
        configParam(A_SHIFT_PARAM, -5.f, 5.f, 0.f, "A Shift (center)", " V");
        configParam(B_SHIFT_PARAM, -5.f, 5.f, 0.f, "B Shift (center)", " V");
        configParam(C_SHIFT_PARAM, -5.f, 5.f, 0.f, "C Shift (center)", " V");
        configParam(D_SHIFT_PARAM, -5.f, 5.f, 0.f, "D Shift (center)", " V");

        // SIZE knobs (0…10 V)
        configParam(A_SIZE_PARAM, 0.f, 10.f, 1.f, "A Size (width)", " V");
        configParam(B_SIZE_PARAM, 0.f, 10.f, 1.f, "B Size (width)", " V");
        configParam(C_SIZE_PARAM, 0.f, 10.f, 1.f, "C Size (width)", " V");
        configParam(D_SIZE_PARAM, 0.f, 10.f, 1.f, "D Size (width)", " V");

        // Inputs
        configInput(A_IN_INPUT, "A In");
        configInput(A_SHIFT_CV_INPUT, "A Shift CV");
        configInput(A_SIZE_CV_INPUT, "A Size CV");

        configInput(B_IN_INPUT, "B In");
        configInput(B_SHIFT_CV_INPUT, "B Shift CV");
        configInput(B_SIZE_CV_INPUT, "B Size CV");

        configInput(C_IN_INPUT, "C In");
        configInput(C_SHIFT_CV_INPUT, "C Shift CV");
        configInput(C_SIZE_CV_INPUT, "C Size CV");

        configInput(D_IN_INPUT, "D In");
        configInput(D_SHIFT_CV_INPUT, "D Shift CV");
        configInput(D_SIZE_CV_INPUT, "D Size CV");

        // Per-channel HI/WIN/LO outputs
        configOutput(A_HI_OUTPUT, "A > window (gate)");
        configOutput(A_WIN_OUTPUT, "A inside window (gate)");
        configOutput(A_LO_OUTPUT, "A < window (gate)");
        configOutput(B_HI_OUTPUT, "B > window (gate)");
        configOutput(B_WIN_OUTPUT, "B inside window (gate)");
        configOutput(B_LO_OUTPUT, "B < window (gate)");
        configOutput(C_HI_OUTPUT, "C > window (gate)");
        configOutput(C_WIN_OUTPUT, "C inside window (gate)");
        configOutput(C_LO_OUTPUT, "C < window (gate)");
        configOutput(D_HI_OUTPUT, "D > window (gate)");
        configOutput(D_WIN_OUTPUT, "D inside window (gate)");
        configOutput(D_LO_OUTPUT, "D < window (gate)");

        // Pair logic outputs
        configOutput(AB_AND_OUTPUT, "A+B AND (both WIN)");
        configOutput(AB_OR_OUTPUT,  "A+B OR (either WIN)");
        configOutput(AB_XOR_OUTPUT, "A+B XOR (one WIN)");
        configOutput(AB_FF_OUTPUT,  "A+B Flip-Flop (XOR rising)");

        configOutput(CD_AND_OUTPUT, "C+D AND (both WIN)");
        configOutput(CD_OR_OUTPUT,  "C+D OR (either WIN)");
        configOutput(CD_XOR_OUTPUT, "C+D XOR (one WIN)");
        configOutput(CD_FF_OUTPUT,  "C+D Flip-Flop (XOR rising)");

        configOutput(PAIRS_AND_OUTPUT, "Pairs AND (A+B active & C+D active)");
        configOutput(PAIRS_OR_OUTPUT,  "Pairs OR (A+B active | C+D active)");
        configOutput(PAIRS_XOR_OUTPUT, "Pairs XOR (exactly one pair active)");

        // Lights
        configLight(A_HI_LIGHT, "A above");
        configLight(A_WIN_LIGHT, "A inside");
        configLight(A_LO_LIGHT, "A below");
        configLight(B_HI_LIGHT, "B above");
        configLight(B_WIN_LIGHT, "B inside");
        configLight(B_LO_LIGHT, "B below");
        configLight(C_HI_LIGHT, "C above");
        configLight(C_WIN_LIGHT, "C inside");
        configLight(C_LO_LIGHT, "C below");
        configLight(D_HI_LIGHT, "D above");
        configLight(D_WIN_LIGHT, "D inside");
        configLight(D_LO_LIGHT, "D below");

        configLight(AB_AND_LIGHT, "A+B AND");
        configLight(AB_OR_LIGHT,  "A+B OR");
        configLight(AB_XOR_LIGHT, "A+B XOR");
        configLight(AB_FF_LIGHT,  "A+B Flip-Flop");

        configLight(CD_AND_LIGHT, "C+D AND");
        configLight(CD_OR_LIGHT,  "C+D OR");
        configLight(CD_XOR_LIGHT, "C+D XOR");
        configLight(CD_FF_LIGHT,  "C+D Flip-Flop");

        configLight(PAIRS_AND_LIGHT, "Pairs AND");
        configLight(PAIRS_OR_LIGHT,  "Pairs OR");
        configLight(PAIRS_XOR_LIGHT, "Pairs XOR");
    }

    void process(const ProcessArgs& args) override {
        const float H = 0.1f; // hysteresis in volts

        // Input normalization A -> B -> C -> D
        float inA = inputs[A_IN_INPUT].getVoltage();
        float inB = inputs[B_IN_INPUT].isConnected() ? inputs[B_IN_INPUT].getVoltage() : inA;
        float inC = inputs[C_IN_INPUT].isConnected() ? inputs[C_IN_INPUT].getVoltage() : inB;
        float inD = inputs[D_IN_INPUT].isConnected() ? inputs[D_IN_INPUT].getVoltage() : inC;

        auto runChannel = [&](float in, int shiftParam, int sizeParam, int shiftCvId, int sizeCvId,
                              bool& hi, bool& win, bool& lo,
                              int oHi, int oWin, int oLo,
                              int lHi, int lWin, int lLo) {
            float center = params[shiftParam].getValue() + inputs[shiftCvId].getVoltage();
            float size   = std::max(0.0001f, params[sizeParam].getValue() + inputs[sizeCvId].getVoltage());
            float hiEdge = center + 0.5f * size;
            float loEdge = center - 0.5f * size;

            if (in > hiEdge + H) {
                hi=true; win=false; lo=false;
            } else if (in < loEdge - H) {
                hi=false; win=false; lo=true;
            } else {
                // inside the extended band; settle toward WIN
                if ((hi && in <= hiEdge - H) || (lo && in >= loEdge + H) || (!win && in >= loEdge + H && in <= hiEdge - H)) {
                    hi=false; win=true; lo=false;
                }
            }

            outputs[oHi].setVoltage(hi ? 10.f : 0.f);
            outputs[oWin].setVoltage(win ? 10.f : 0.f);
            outputs[oLo].setVoltage(lo ? 10.f : 0.f);

            lights[lHi].setBrightnessSmooth(hi ? 1.f : 0.f, args.sampleTime);
            lights[lWin].setBrightnessSmooth(win ? 1.f : 0.f, args.sampleTime);
            lights[lLo].setBrightnessSmooth(lo ? 1.f : 0.f, args.sampleTime);
        };

        runChannel(inA, A_SHIFT_PARAM, A_SIZE_PARAM, A_SHIFT_CV_INPUT, A_SIZE_CV_INPUT,
                   aHi, aWin, aLo, A_HI_OUTPUT, A_WIN_OUTPUT, A_LO_OUTPUT, A_HI_LIGHT, A_WIN_LIGHT, A_LO_LIGHT);
        runChannel(inB, B_SHIFT_PARAM, B_SIZE_PARAM, B_SHIFT_CV_INPUT, B_SIZE_CV_INPUT,
                   bHi, bWin, bLo, B_HI_OUTPUT, B_WIN_OUTPUT, B_LO_OUTPUT, B_HI_LIGHT, B_WIN_LIGHT, B_LO_LIGHT);
        runChannel(inC, C_SHIFT_PARAM, C_SIZE_PARAM, C_SHIFT_CV_INPUT, C_SIZE_CV_INPUT,
                   cHi, cWin, cLo, C_HI_OUTPUT, C_WIN_OUTPUT, C_LO_OUTPUT, C_HI_LIGHT, C_WIN_LIGHT, C_LO_LIGHT);
        runChannel(inD, D_SHIFT_PARAM, D_SIZE_PARAM, D_SHIFT_CV_INPUT, D_SIZE_CV_INPUT,
                   dHi, dWin, dLo, D_HI_OUTPUT, D_WIN_OUTPUT, D_LO_OUTPUT, D_HI_LIGHT, D_WIN_LIGHT, D_LO_LIGHT);

        // Pair logic
        bool abAnd = aWin && bWin;
        bool abOr  = aWin || bWin;
        bool abXor = aWin != bWin;

        outputs[AB_AND_OUTPUT].setVoltage(abAnd ? 10.f : 0.f);
        outputs[AB_OR_OUTPUT].setVoltage(abOr ? 10.f : 0.f);
        outputs[AB_XOR_OUTPUT].setVoltage(abXor ? 10.f : 0.f);

        lights[AB_AND_LIGHT].setBrightnessSmooth(abAnd ? 1.f : 0.f, args.sampleTime);
        lights[AB_OR_LIGHT].setBrightnessSmooth(abOr ? 1.f : 0.f, args.sampleTime);
        lights[AB_XOR_LIGHT].setBrightnessSmooth(abXor ? 1.f : 0.f, args.sampleTime);

        if (abXor && !abXorPrevious) abFlipFlop = !abFlipFlop;
        abXorPrevious = abXor;
        outputs[AB_FF_OUTPUT].setVoltage(abFlipFlop ? 10.f : 0.f);
        lights[AB_FF_LIGHT].setBrightnessSmooth(abFlipFlop ? 1.f : 0.f, args.sampleTime);

        bool cdAnd = cWin && dWin;
        bool cdOr  = cWin || dWin;
        bool cdXor = cWin != dWin;

        outputs[CD_AND_OUTPUT].setVoltage(cdAnd ? 10.f : 0.f);
        outputs[CD_OR_OUTPUT].setVoltage(cdOr ? 10.f : 0.f);
        outputs[CD_XOR_OUTPUT].setVoltage(cdXor ? 10.f : 0.f);

        lights[CD_AND_LIGHT].setBrightnessSmooth(cdAnd ? 1.f : 0.f, args.sampleTime);
        lights[CD_OR_LIGHT].setBrightnessSmooth(cdOr ? 1.f : 0.f, args.sampleTime);
        lights[CD_XOR_LIGHT].setBrightnessSmooth(cdXor ? 1.f : 0.f, args.sampleTime);

        if (cdXor && !cdXorPrevious) cdFlipFlop = !cdFlipFlop;
        cdXorPrevious = cdXor;
        outputs[CD_FF_OUTPUT].setVoltage(cdFlipFlop ? 10.f : 0.f);
        lights[CD_FF_LIGHT].setBrightnessSmooth(cdFlipFlop ? 1.f : 0.f, args.sampleTime);

        bool abActive = abAnd || abOr || abXor;
        bool cdActive = cdAnd || cdOr || cdXor;

        bool pairsAnd = abActive && cdActive;
        bool pairsOr  = abActive || cdActive;
        bool pairsXor = abActive != cdActive;

        outputs[PAIRS_AND_OUTPUT].setVoltage(pairsAnd ? 10.f : 0.f);
        outputs[PAIRS_OR_OUTPUT].setVoltage(pairsOr ? 10.f : 0.f);
        outputs[PAIRS_XOR_OUTPUT].setVoltage(pairsXor ? 10.f : 0.f);

        lights[PAIRS_AND_LIGHT].setBrightnessSmooth(pairsAnd ? 1.f : 0.f, args.sampleTime);
        lights[PAIRS_OR_LIGHT].setBrightnessSmooth(pairsOr ? 1.f : 0.f, args.sampleTime);
        lights[PAIRS_XOR_LIGHT].setBrightnessSmooth(pairsXor ? 1.f : 0.f, args.sampleTime);
    }
};

struct ComparallyWidget : ModuleWidget {
    ComparallyWidget(Comparally* module) {
        setModule(module);

        // 20HP panel (101.6 mm)
        box.size = Vec(RACK_GRID_WIDTH * 20, RACK_GRID_HEIGHT);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Comparally.svg")));

        // ======= EXACT COORDINATES FROM SVG ANALYSIS (mm) =======
        // Base coordinates from SVG with transforms applied

        // Knob columns (from SVG KNOB elements)
        const float xA = 15.000000f;        // KNOB-4, KNOB-4-8
        const float xB = 30.044950f;        // KNOB, KNOB-8  
        const float xC = 45.089901f;        // KNOB-6, KNOB-6-4
        const float xD = 60.134850f;        // KNOB-1, KNOB-1-5

        // Knob rows (from SVG KNOB elements)
        const float ySHIFT = 19.000000f;    // First row of knobs
        const float ySIZE  = 34.000000f;    // Second row of knobs

        // Input jacks (from SVG JACK elements)
        const float yIN        = 49.186512f;  // JACK, JACK-5, JACK-7, JACK-53
        const float ySHIFT_CV  = 60.281250f;  // JACK-4, JACK-5-6, JACK-7-7, JACK-53-5
        const float ySIZE_CV   = 71.375984f;  // JACK-0, JACK-5-7, JACK-7-79, JACK-53-7

        // Output jacks (from SVG JACK elements)
        const float yHI_JACK   = 87.057114f;  // JACK-0-4, JACK-5-7-9, JACK-7-79-1, JACK-53-7-5
        const float yWIN_JACK  = 98.814629f;  // JACK-0-4-9, JACK-5-7-9-0, JACK-7-79-1-7, JACK-53-7-5-6
        const float yLO_JACK   = 110.572140f; // JACK-0-4-1, JACK-5-7-9-9, JACK-7-79-1-6, JACK-53-7-5-7

        // LED positions (from SVG LED elements)
        const float xA_LED = 20.110708f;    // LED, LED-1, LED-1-6
        const float xB_LED = 35.197609f;    // LED-1-0, LED-9, LED-6
        const float xC_LED = 50.284512f;    // LED-1-3, LED-7, LED-6-3
        const float xD_LED = 65.371414f;    // LED-1-8, LED-98, LED-6-39

        const float yHI_LED  = 89.557114f;  // LED, LED-6, LED-6-3, LED-6-39
        const float yWIN_LED = 101.314640f; // LED-1, LED-9, LED-7, LED-98
        const float yLO_LED  = 113.072140f; // LED-1-6, LED-1-0, LED-1-3, LED-1-8

        // Right logic section (from SVG JACK elements with transforms)
        // Base coordinates: JACK-53-7-5 (cx="60.13485", cy="87.057114")
        // Transform groups create the logic section positions
        const float xLOGIC_L = 60.134850f + 20.446762f;  // Base + transform: 80.581612f
        const float xLOGIC_R = 60.134850f + 32.131813f;  // Base + transform: 92.266663f
        const float xLOGIC_M = 60.134850f + 26.378906f;  // Base + transform: 86.513756f

        // Y positions from transforms applied to base cy="87.057114"
        const float yL0 = 87.057114f - 19.761284f;  // 67.295830f
        const float yL1 = 87.057114f - 33.844615f;  // 53.212499f  
        const float yL2 = 87.057114f - 52.116667f;  // 34.940447f
        const float yL3 = 87.057114f - 67.411277f;  // 19.645837f

        // Pairs group Y positions
        const float yP0 = 87.057114f;               // Base position
        const float yP1 = 87.057114f + 11.757515f;  // 98.814629f
        const float yP2 = 87.057114f + 23.515030f;  // 110.572144f

        // Right logic LEDs (from SVG LED elements with transforms)
        // Base coordinates: LED-6-39 (cx="65.371414", cy="89.557114")
        const float xLED_L = 65.371414f + 20.446762f;  // 85.818176f
        const float xLED_R = 65.371414f + 32.131813f;  // 97.503227f
        const float xLED_M = 65.371414f + 26.378906f;  // 91.750320f

        // Y positions from transforms applied to base cy="89.557114"
        const float yLED_L0 = 89.557114f - 19.761284f;  // 69.795830f
        const float yLED_L1 = 89.557114f - 33.844615f;  // 55.712499f
        const float yLED_L2 = 89.557114f - 52.116667f;  // 37.440447f
        const float yLED_L3 = 89.557114f - 67.411277f;  // 22.145837f

        const float yLED_P0 = 89.557114f;               // Base position
        const float yLED_P1 = 89.557114f + 11.757515f;  // 101.314629f
        const float yLED_P2 = 89.557114f + 23.515030f;  // 113.072144f

        // ======= CONTROLS =======

        // SHIFT knobs (CustomKnob per request)
        addParam(createParamCentered<CustomKnob>(mm2px(Vec(xA, ySHIFT)), module, Comparally::A_SHIFT_PARAM));
        addParam(createParamCentered<CustomKnob>(mm2px(Vec(xB, ySHIFT)), module, Comparally::B_SHIFT_PARAM));
        addParam(createParamCentered<CustomKnob>(mm2px(Vec(xC, ySHIFT)), module, Comparally::C_SHIFT_PARAM));
        addParam(createParamCentered<CustomKnob>(mm2px(Vec(xD, ySHIFT)), module, Comparally::D_SHIFT_PARAM));

        // SIZE knobs
        addParam(createParamCentered<CustomKnob>(mm2px(Vec(xA, ySIZE)), module, Comparally::A_SIZE_PARAM));
        addParam(createParamCentered<CustomKnob>(mm2px(Vec(xB, ySIZE)), module, Comparally::B_SIZE_PARAM));
        addParam(createParamCentered<CustomKnob>(mm2px(Vec(xC, ySIZE)), module, Comparally::C_SIZE_PARAM));
        addParam(createParamCentered<CustomKnob>(mm2px(Vec(xD, ySIZE)), module, Comparally::D_SIZE_PARAM));

        // Inputs A–D
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xA, yIN)), module, Comparally::A_IN_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xB, yIN)), module, Comparally::B_IN_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xC, yIN)), module, Comparally::C_IN_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xD, yIN)), module, Comparally::D_IN_INPUT));

        // SHIFT CVs
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xA, ySHIFT_CV)), module, Comparally::A_SHIFT_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xB, ySHIFT_CV)), module, Comparally::B_SHIFT_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xC, ySHIFT_CV)), module, Comparally::C_SHIFT_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xD, ySHIFT_CV)), module, Comparally::D_SHIFT_CV_INPUT));

        // SIZE CVs
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xA, ySIZE_CV)), module, Comparally::A_SIZE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xB, ySIZE_CV)), module, Comparally::B_SIZE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xC, ySIZE_CV)), module, Comparally::C_SIZE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(xD, ySIZE_CV)), module, Comparally::D_SIZE_CV_INPUT));

        // HI / WIN / LO outputs + LEDs (left grid)
        // HI
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xA, yHI_JACK)), module, Comparally::A_HI_OUTPUT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(xA_LED, yHI_LED)), module, Comparally::A_HI_LIGHT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xB, yHI_JACK)), module, Comparally::B_HI_OUTPUT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(xB_LED, yHI_LED)), module, Comparally::B_HI_LIGHT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xC, yHI_JACK)), module, Comparally::C_HI_OUTPUT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(xC_LED, yHI_LED)), module, Comparally::C_HI_LIGHT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xD, yHI_JACK)), module, Comparally::D_HI_OUTPUT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(xD_LED, yHI_LED)), module, Comparally::D_HI_LIGHT));

        // WIN
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xA, yWIN_JACK)), module, Comparally::A_WIN_OUTPUT));
        addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(xA_LED, yWIN_LED)), module, Comparally::A_WIN_LIGHT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xB, yWIN_JACK)), module, Comparally::B_WIN_OUTPUT));
        addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(xB_LED, yWIN_LED)), module, Comparally::B_WIN_LIGHT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xC, yWIN_JACK)), module, Comparally::C_WIN_OUTPUT));
        addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(xC_LED, yWIN_LED)), module, Comparally::C_WIN_LIGHT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xD, yWIN_JACK)), module, Comparally::D_WIN_OUTPUT));
        addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(xD_LED, yWIN_LED)), module, Comparally::D_WIN_LIGHT));

        // LO
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xA, yLO_JACK)), module, Comparally::A_LO_OUTPUT));
        addChild(createLightCentered<SmallLight<BlueLight>>(mm2px(Vec(xA_LED, yLO_LED)), module, Comparally::A_LO_LIGHT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xB, yLO_JACK)), module, Comparally::B_LO_OUTPUT));
        addChild(createLightCentered<SmallLight<BlueLight>>(mm2px(Vec(xB_LED, yLO_LED)), module, Comparally::B_LO_LIGHT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xC, yLO_JACK)), module, Comparally::C_LO_OUTPUT));
        addChild(createLightCentered<SmallLight<BlueLight>>(mm2px(Vec(xC_LED, yLO_LED)), module, Comparally::C_LO_LIGHT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xD, yLO_JACK)), module, Comparally::D_LO_OUTPUT));
        addChild(createLightCentered<SmallLight<BlueLight>>(mm2px(Vec(xD_LED, yLO_LED)), module, Comparally::D_LO_LIGHT));

        // ===== Right-side Logic =====
        // Top 4 jacks (rows 1-2): A+B logic (left and right columns)
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xLOGIC_L, yL0)), module, Comparally::AB_AND_OUTPUT));
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(xLED_L,  yLED_L0)), module, Comparally::AB_AND_LIGHT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xLOGIC_R, yL0)), module, Comparally::AB_OR_OUTPUT));
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(xLED_R,  yLED_L0)), module, Comparally::AB_OR_LIGHT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xLOGIC_L, yL1)), module, Comparally::AB_XOR_OUTPUT));
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(xLED_L,  yLED_L1)), module, Comparally::AB_XOR_LIGHT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xLOGIC_R, yL1)), module, Comparally::AB_FF_OUTPUT));
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(xLED_R,  yLED_L1)), module, Comparally::AB_FF_LIGHT));

        // Bottom 4 jacks (rows 3-4): C+D logic (left and right columns)
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xLOGIC_L, yL2)), module, Comparally::CD_AND_OUTPUT));
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(xLED_L,  yLED_L2)), module, Comparally::CD_AND_LIGHT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xLOGIC_R, yL2)), module, Comparally::CD_OR_OUTPUT));
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(xLED_R,  yLED_L2)), module, Comparally::CD_OR_LIGHT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xLOGIC_L, yL3)), module, Comparally::CD_XOR_OUTPUT));
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(xLED_L,  yLED_L3)), module, Comparally::CD_XOR_LIGHT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xLOGIC_R, yL3)), module, Comparally::CD_FF_OUTPUT));
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(xLED_R,  yLED_L3)), module, Comparally::CD_FF_LIGHT));

        // Pairs group (middle column of 3)
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xLOGIC_M, yP0)), module, Comparally::PAIRS_AND_OUTPUT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(xLED_M,    yLED_P0)), module, Comparally::PAIRS_AND_LIGHT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xLOGIC_M, yP1)), module, Comparally::PAIRS_OR_OUTPUT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(xLED_M,    yLED_P1)), module, Comparally::PAIRS_OR_LIGHT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(xLOGIC_M, yP2)), module, Comparally::PAIRS_XOR_OUTPUT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(xLED_M,    yLED_P2)), module, Comparally::PAIRS_XOR_LIGHT));

        // Add VCV Rack mounting screws
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    }
};

Model* modelComparally = createModel<Comparally, ComparallyWidget>("Comparally");
