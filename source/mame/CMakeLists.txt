cmake_minimum_required(VERSION 3.15)

# Machines emulated by MAME, run through the mp_* API of a MAME build made
# with OSD=plugin (libmame_<subtarget>.so, see src/osd/plugin in MAME)

set(MAME_PLUGIN_LIBRARY "" CACHE FILEPATH "MAME plugin library built with OSD=plugin, e.g. libmame_synths.so")
set(MAME_PLUGIN_INCLUDE_DIR "" CACHE PATH "Directory containing mame_plugin.h (src/osd/plugin in the MAME tree)")

if(NOT MAME_PLUGIN_LIBRARY OR NOT EXISTS "${MAME_PLUGIN_LIBRARY}")
	message(FATAL_ERROR "MAME: set MAME_PLUGIN_LIBRARY to the MAME plugin library (libmame_<subtarget>.so)")
endif()
if(NOT MAME_PLUGIN_INCLUDE_DIR OR NOT EXISTS "${MAME_PLUGIN_INCLUDE_DIR}/mame_plugin.h")
	message(FATAL_ERROR "MAME: set MAME_PLUGIN_INCLUDE_DIR to the directory containing mame_plugin.h")
endif()

add_library(mamePluginApi SHARED IMPORTED GLOBAL)
set_target_properties(mamePluginApi PROPERTIES
	IMPORTED_LOCATION "${MAME_PLUGIN_LIBRARY}"
	INTERFACE_INCLUDE_DIRECTORIES "${MAME_PLUGIN_INCLUDE_DIR}")

add_subdirectory(mameLib)

# CtrlrX runs panels for the machines inside the plugins, see docs/ctrlrx.md of muse
set(MAME_CTRLR_DIR "" CACHE PATH "CtrlrX fork with the muse/ folder, to build the plugins with panels")

if(MAME_CTRLR_DIR AND ${CMAKE_PROJECT_NAME}_BUILD_JUCEPLUGIN)
	# the modules CtrlrX needs beyond those of the other plugins. Its panels open modal windows from Lua
	target_link_libraries(juce_plugin_modules PRIVATE juce::juce_gui_extra juce::juce_product_unlocking)
	target_compile_definitions(juce_plugin_modules PUBLIC JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_MODULE_AVAILABLE_juce_product_unlocking=1)
	set(CTRLR_JUCE_MODULES_TARGET juce_plugin_modules)
	add_subdirectory(${MAME_CTRLR_DIR}/muse ctrlrx)
endif()

if(${CMAKE_PROJECT_NAME}_BUILD_JUCEPLUGIN)
	include(mameJucePlugin/mamePlugin.cmake)
	add_subdirectory(tx81zJucePlugin)
	add_subdirectory(ex20JucePlugin)
endif()
