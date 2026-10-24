cmake_minimum_required(VERSION 3.15)

project(mameJucePlugin VERSION ${CMAKE_PROJECT_VERSION})

set(SOURCES
	mameController.cpp mameController.h
	mamePluginProcessor.cpp mamePluginProcessor.h
	parameterDescriptions_mame.json

	skins/mameDefault/mameDefault.rml
	skins/mameDefault/mameDefault.rcss
)

SET(ASSETS "parameterDescriptions_mame.json")

addSkin("MAME-TX81Z" "mameDefault" "skins/mameDefault" "mameDefault.rml")

buildSkinHeader()

juce_add_binary_data(mameJucePlugin_BinaryData SOURCES ${ASSETS} ${ASSETS_SKINS})

createJucePluginWithFX(mameJucePlugin "MAME-TX81Z" "Tmtx" "Tmtf" mameJucePlugin_BinaryData mameLib)

# the machine this plugin runs
target_compile_definitions(mameJucePlugin PRIVATE MAME_MACHINE="tx81z")

# find the MAME library where it was built; bundling it with the plugins is still to do
get_filename_component(MAME_PLUGIN_LIBRARY_DIR "${MAME_PLUGIN_LIBRARY}" DIRECTORY)
foreach(format VST3 CLAP LV2 AU Standalone)
	if(TARGET mameJucePlugin_${format})
		set_target_properties(mameJucePlugin_${format} PROPERTIES BUILD_RPATH "${MAME_PLUGIN_LIBRARY_DIR}")
	endif()
endforeach()
