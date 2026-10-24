cmake_minimum_required(VERSION 3.15)

project(mameJucePlugin VERSION ${CMAKE_PROJECT_VERSION})

set(SOURCES
	mameController.cpp mameController.h
	mameEditor.cpp mameEditor.h
	mamePanel.cpp mamePanel.h
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

# the MAME library goes next to each plugin binary, which finds it there. It is copied before linking because
# the VST3 and LV2 helpers load the plugin right after
foreach(format VST3 CLAP LV2 Standalone)
	if(TARGET mameJucePlugin_${format})
		set_target_properties(mameJucePlugin_${format} PROPERTIES INSTALL_RPATH "$ORIGIN" BUILD_WITH_INSTALL_RPATH ON)
		add_custom_command(TARGET mameJucePlugin_${format} PRE_LINK
			COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:mameJucePlugin_${format}>"
			COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MAME_PLUGIN_LIBRARY}" "$<TARGET_FILE_DIR:mameJucePlugin_${format}>")
	endif()
endforeach()

# VST3 and LV2 bundles are installed as directories and take it along, the other formats are single files
if(UNIX AND NOT APPLE)
	if(TARGET mameJucePlugin_CLAP)
		install(FILES "${MAME_PLUGIN_LIBRARY}" DESTINATION lib/clap/ COMPONENT MAME-TX81Z-CLAP)
	endif()
endif()
if(TARGET mameJucePlugin_Standalone)
	install(FILES "${MAME_PLUGIN_LIBRARY}" DESTINATION . COMPONENT MAME-TX81Z-Standalone)
endif()
