# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\MachineLearningDemo_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\MachineLearningDemo_autogen.dir\\ParseCache.txt"
  "MachineLearningDemo_autogen"
  )
endif()
