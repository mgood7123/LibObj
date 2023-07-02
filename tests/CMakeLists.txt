cmake_minimum_required(VERSION 3.4.1)

# disable example for now, focus on LibObj

#testBuilder_add_source(example example.cpp)
#testBuilder_add_library(example gtest_main)
#testBuilder_build(example EXECUTABLES)

testBuilder_add_source(LibObj_Tests LibObj_Tests.cpp)
testBuilder_add_library(LibObj_Tests gtest_main)
testBuilder_add_library(LibObj_Tests LibObj)
testBuilder_build(LibObj_Tests EXECUTABLES)