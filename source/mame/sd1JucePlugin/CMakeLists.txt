cmake_minimum_required(VERSION 3.15)

project(sd1JucePlugin VERSION ${CMAKE_PROJECT_VERSION})

createMamePlugin(sd1JucePlugin "MAME-SD1" "sd132" "Tmsd" "Tmsf")
