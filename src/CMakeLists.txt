file(GLOB SOURCES
    ./*.h
    ./*.cpp
)

add_executable(metal_engine
    ${SOURCES}
    utility/fileIO.cpp
    utility/fileIO.h
)

target_include_directories(metal_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(metal_engine
    METAL_CPP
)