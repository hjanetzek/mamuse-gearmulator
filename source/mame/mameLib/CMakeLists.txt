cmake_minimum_required(VERSION 3.10)

project(mameLib)

add_library(mameLib STATIC)

set(SOURCES
	device.cpp device.h
	settings.cpp settings.h
)

target_sources(mameLib PRIVATE ${SOURCES})
source_group("source" FILES ${SOURCES})

set_property(TARGET mameLib PROPERTY FOLDER "MAME")

target_link_libraries(mameLib PUBLIC synthLib mamePluginApi)

target_include_directories(mameLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
