cmake_minimum_required(VERSION 3.20)
project(CadCertifier LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FRONTIER_ROOT "" CACHE PATH "Frontier source with the current SolidArc overlay applied")
if(NOT EXISTS "${FRONTIER_ROOT}/Engine/GeometryProcessing/CMakeLists.txt")
    message(FATAL_ERROR "Set -DFRONTIER_ROOT=/path/to/Frontier")
endif()
set(SOLIDARC_UI "${FRONTIER_ROOT}/UI/SolidArc")
set(SOLIDARC_PACKAGES "${FRONTIER_ROOT}/ExternalPackages")

add_subdirectory("${FRONTIER_ROOT}/Engine/GeometryProcessing" "${CMAKE_BINARY_DIR}/GeometryProcessing")

add_library(FrontierOrbitCamera STATIC "${FRONTIER_ROOT}/Engine/Render/OrbitCamera.cpp")
target_include_directories(FrontierOrbitCamera PUBLIC "${FRONTIER_ROOT}")

add_library(SolidArcTomlPlusPlus INTERFACE)
target_include_directories(SolidArcTomlPlusPlus SYSTEM INTERFACE "${SOLIDARC_PACKAGES}/tomlpp/include")

add_library(SolidArcStb INTERFACE)
target_include_directories(SolidArcStb SYSTEM INTERFACE "${SOLIDARC_PACKAGES}/stb")

add_library(FrontierSolidArcDocument STATIC
    ${SOLIDARC_UI}/Document/SolidArcDocument.cpp
    ${SOLIDARC_UI}/Document/SolidArcDocumentCodec.cpp
    ${SOLIDARC_UI}/Document/VehicleCurveImport.cpp
    ${SOLIDARC_UI}/Document/SolidArcImagePlane.cpp
    ${SOLIDARC_UI}/Document/SolidArcMatcap.cpp
    ${SOLIDARC_UI}/Document/SolidArcMeasurement.cpp
    ${SOLIDARC_UI}/Document/SolidArcDrawingSheet.cpp
    ${SOLIDARC_UI}/Document/SolidArcExchange.cpp
    ${SOLIDARC_UI}/Document/SolidArcKnife.cpp
    ${SOLIDARC_UI}/Document/SolidArcScene.cpp
    ${SOLIDARC_UI}/Document/SolidArcSpecimen.cpp
)
target_include_directories(FrontierSolidArcDocument PUBLIC "${FRONTIER_ROOT}")
target_link_libraries(FrontierSolidArcDocument PUBLIC FrontierGeometryProcessing FrontierOrbitCamera SolidArcTomlPlusPlus SolidArcStb)

add_executable(CadCertifier Certifier.cpp)
target_link_libraries(CadCertifier PRIVATE FrontierSolidArcDocument)
