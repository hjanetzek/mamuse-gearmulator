cmake_minimum_required(VERSION 3.15)

project(ex20JucePlugin VERSION ${CMAKE_PROJECT_VERSION})

createMamePlugin(ex20JucePlugin "MAME-EX20" "ex20" "Tmex" "Tmef")
