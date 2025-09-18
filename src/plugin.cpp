// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2025 ifnoon
// Part of the ifnoon VCV Rack plugin project.

/*
 * plugin.cpp - VCV Rack Plugin Entry Point for ifnoon Modules
 * 
 * This file contains the plugin initialization and module registration
 * for the ifnoon plugin collection including Comparally module.
 */

#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;
    
    // Register Comparally module
    p->addModel(modelComparally);
}

