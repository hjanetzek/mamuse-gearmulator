cmake_minimum_required(VERSION 3.15)

project(linndrumJucePlugin VERSION ${CMAKE_PROJECT_VERSION})

createMamePlugin(linndrumJucePlugin "MAME-LinnDrum" "linndrum" "Tmld" "Tmlf")
