// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 ifnoon
// Part of the ifnoon VCV Rack plugin project.

/*
 * CustomKnob.hpp - Custom Knob Implementation for ifnoon Modules
 * 
 * This file defines a custom knob class that extends VCV Rack's SvgKnob
 * with specific behavior for the ifnoon modules:
 * - 270° rotation range (like real potentiometers)
 * - No drop shadows for clean appearance
 * - Smooth continuous rotation without snapping
 */

#pragma once

#include "rack.hpp"

using namespace rack;

struct CustomKnob : SvgKnob {
    CustomKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/knob_custom.svg")));
        
        // Limit rotation like a real potentiometer (270° instead of 360°)
        minAngle = -0.75 * M_PI;  // -135°
        maxAngle = 0.75 * M_PI;   // +135°
        
        // Smooth continuous rotation (no snapping)
        snap = false;
        smooth = true;
    }
    
    void onDoubleClick(const DoubleClickEvent& e) override {
        // Reset to default value on double-click
        if (getParamQuantity()) {
            getParamQuantity()->reset();
        }
        SvgKnob::onDoubleClick(e);
    }
};