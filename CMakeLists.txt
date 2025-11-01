# --------------------------------------------------------------------------
# Dependencies
# --------------------------------------------------------------------------

find_package(yat CONFIG REQUIRED)
find_package(eigerapi CONFIG REQUIRED)
find_package(ZeroMQ CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)

# --------------------------------------------------------------------------
# Collect sources and includes
# --------------------------------------------------------------------------

file(GLOB_RECURSE sources 
    src/*.cpp      
    src/bitshuffle-master/*.c
)
set(includedirs
    include/
)

# --------------------------------------------------------------------------
# Target
# --------------------------------------------------------------------------

add_library(limaeiger ${sources})

target_include_directories(limaeiger
    PUBLIC 
        ${includedirs}
)

target_link_libraries(limaeiger 
    PUBLIC
        limacore
        yat::yat
        eigerapi::eigerapi
    PRIVATE
        lz4::lz4
        libzmq
)

limatools_set_library_soversion(limaeiger "VERSION")

# --------------------------------------------------------------------------
# Compile definitions
# --------------------------------------------------------------------------

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" EIGER_VERSION_RAW)
string(STRIP "${EIGER_VERSION_RAW}" EIGER_VERSION)

target_compile_definitions(limaeiger
    PUBLIC
        EIGER_NAME=Eiger
        EIGER_VERSION=${EIGER_VERSION}
        EIGER_ENABLED
)

message(STATUS "Camera enabled: Eiger ${EIGER_VERSION}")

if (CONAN_LIBCXX STREQUAL "libstdc++")
    target_compile_features(limaeiger 
        PRIVATE 
            cxx_std_11
    )
endif()

# --------------------------------------------------------------------------
# Install
# --------------------------------------------------------------------------

install(
    TARGETS limaeiger 
    LIBRARY DESTINATION lib
)
