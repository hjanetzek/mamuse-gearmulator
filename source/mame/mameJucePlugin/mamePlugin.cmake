# One plugin product per MAME machine. The sources here are compiled into each product, which lives in a folder of its
# own with the skin showing the machine's panel, e.g.
#
#   createMamePlugin(tx81zJucePlugin "MAME-TX81Z" "tx81z" "Tmtx" "Tmtf")
#
# Each product has a skin "mameDefault" in skins/mameDefault of its folder

set(MAME_JUCE_PLUGIN_DIR ${CMAKE_CURRENT_LIST_DIR})

macro(createMamePlugin targetName productName machine plugin4CCSynth plugin4CCFX)
	set(SOURCES
		${MAME_JUCE_PLUGIN_DIR}/mameController.cpp ${MAME_JUCE_PLUGIN_DIR}/mameController.h
		${MAME_JUCE_PLUGIN_DIR}/mameEditor.cpp ${MAME_JUCE_PLUGIN_DIR}/mameEditor.h
		${MAME_JUCE_PLUGIN_DIR}/mameLayout.cpp ${MAME_JUCE_PLUGIN_DIR}/mameLayout.h
		${MAME_JUCE_PLUGIN_DIR}/mamePanel.cpp ${MAME_JUCE_PLUGIN_DIR}/mamePanel.h
		${MAME_JUCE_PLUGIN_DIR}/mamePluginProcessor.cpp ${MAME_JUCE_PLUGIN_DIR}/mamePluginProcessor.h
		${MAME_JUCE_PLUGIN_DIR}/parameterDescriptions_mame.json

		skins/mameDefault/mameDefault.rml
		skins/mameDefault/mameDefault.rcss
	)

	# CtrlrX panels, see MAME_CTRLR_DIR
	if(TARGET ctrlrLib)
		list(APPEND SOURCES ${MAME_JUCE_PLUGIN_DIR}/mameCtrlr.cpp ${MAME_JUCE_PLUGIN_DIR}/mameCtrlr.h)
	endif()

	set(ASSETS "${MAME_JUCE_PLUGIN_DIR}/parameterDescriptions_mame.json")

	addSkin("${productName}" "mameDefault" "skins/mameDefault" "mameDefault.rml")

	buildSkinHeader()

	juce_add_binary_data(${targetName}_BinaryData SOURCES ${ASSETS} ${ASSETS_SKINS})

	createJucePluginWithFX(${targetName} "${productName}" "${plugin4CCSynth}" "${plugin4CCFX}" ${targetName}_BinaryData mameLib)

	foreach(variant ${targetName} ${targetName}_FX)
		if(TARGET ${variant})
			# the machine this plugin runs
			target_compile_definitions(${variant} PRIVATE MAME_MACHINE="${machine}")

			if(TARGET ctrlrLib)
				target_link_libraries(${variant} PRIVATE ctrlrLib)
				target_compile_definitions(${variant} PRIVATE MAME_WITH_CTRLR=1)
			endif()
		endif()

		# the MAME library goes next to each plugin binary, which finds it there. It is copied before linking because
		# the VST3 and LV2 helpers load the plugin right after
		foreach(format VST3 CLAP LV2 Standalone)
			if(TARGET ${variant}_${format})
				set_target_properties(${variant}_${format} PROPERTIES INSTALL_RPATH "$ORIGIN" BUILD_WITH_INSTALL_RPATH ON)
				# the plugin links libstdc++ statically while the MAME library uses the shared one. Exported, the static
				# copy's symbols would be bound by the MAME library too, mixing both copies
				if(UNIX AND NOT APPLE)
					target_link_options(${variant}_${format} PRIVATE "LINKER:--exclude-libs,libstdc++.a:libgcc.a:libgcc_eh.a")
				endif()
				add_custom_command(TARGET ${variant}_${format} PRE_LINK
					COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${variant}_${format}>"
					COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MAME_PLUGIN_LIBRARY}" "$<TARGET_FILE_DIR:${variant}_${format}>")
			endif()
		endforeach()

		# VST3 and LV2 bundles are installed as directories and take it along, the other formats are single files
		if(variant STREQUAL "${targetName}")
			set(variantProduct "${productName}")
		else()
			set(variantProduct "${productName}FX")
		endif()
		if(TARGET ${variant}_CLAP AND UNIX AND NOT APPLE)
			install(FILES "${MAME_PLUGIN_LIBRARY}" DESTINATION lib/clap/ COMPONENT ${variantProduct}-CLAP)
		endif()
		if(TARGET ${variant}_Standalone)
			install(FILES "${MAME_PLUGIN_LIBRARY}" DESTINATION . COMPONENT ${variantProduct}-Standalone)
		endif()
	endforeach()
endmacro()
