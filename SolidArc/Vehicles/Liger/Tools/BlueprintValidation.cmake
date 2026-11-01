# Include after project() with -DCMAKE_PROJECT_INCLUDE=/absolute/path/to/this/file.
# The FrontierSolidArcUI target is resolved when the host finishes configuring.
if(CMAKE_PROJECT_NAME STREQUAL "FrontierUIFoundation" AND NOT TARGET LigerBlueprintValidation)
    add_executable(LigerBlueprintValidation "${CMAKE_CURRENT_LIST_DIR}/BlueprintValidation.cpp")
    target_link_libraries(LigerBlueprintValidation PRIVATE FrontierSolidArcUI)
endif()
if(CMAKE_PROJECT_NAME STREQUAL "FrontierUIFoundation" AND NOT TARGET LigerCadCertifier)
    add_executable(LigerCadCertifier "${CMAKE_CURRENT_LIST_DIR}/Certifier.cpp")
    target_link_libraries(LigerCadCertifier PRIVATE FrontierSolidArcDocument)
endif()
