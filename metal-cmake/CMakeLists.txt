add_library(METAL_CPP
    ${CMAKE_CURRENT_SOURCE_DIR}/definition.cpp
)

find_library(APPLE_FWK_FOUNDATION Foundation REQUIRED)
find_library(APPLE_FWK_QUARTS_CORE QuartzCore REQUIRED)
find_library(APPLE_FWK_METAL Metal REQUIRED)
find_library(APPLE_FWK_APP_KIT AppKit REQUIRED)
find_library(APPLE_FWK_METAL_KIT MetalKit REQUIRED)

target_include_directories(METAL_CPP PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/metal-cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/metal-cpp-extensions"
)

target_link_libraries(METAL_CPP
    ${APPLE_FWK_FOUNDATION}
    ${APPLE_FWK_METAL}
    ${APPLE_FWK_METAL_KIT}
    ${APPLE_FWK_QUARTS_CORE}
    ${APPLE_FWK_APP_KIT}
)

message(STATUS "Finished metal-cpp library")
