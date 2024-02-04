get_filename_component(farbot_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

if(NOT TARGET farbot::farbot)
    include("${farbot_CMAKE_DIR}/farbot-targets.cmake")
endif()
