cmake_minimum_required(VERSION 3.15)
project(gocxx VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)  # Use C++17 for std::optional and modern features
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch nlohmann/json for JSON functionality
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

# Options
option(GOCXX_ENABLE_TESTS "Enable building of tests (requires GTest)" OFF)
option(GOCXX_ENABLE_DOCS "Enable building of documentation (requires Doxygen)" OFF)



# ---------------------------------------------------
# --- Define gocxx as the main static bundle --------
# ---------------------------------------------------

# Collect all source files
file(GLOB_RECURSE GOCXX_SOURCES 
    "src/*.cpp"
)

# Collect all header files
file(GLOB_RECURSE GOCXX_HEADERS 
    "include/*.h"
)

add_library(gocxx STATIC
    ${GOCXX_SOURCES}
    ${GOCXX_HEADERS}
)

target_include_directories(gocxx PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link with nlohmann_json
target_link_libraries(gocxx PUBLIC nlohmann_json::nlohmann_json)

# This is needed to ensure relocatable static linking
set_target_properties(gocxx PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------
# --- Install + pkg-config --------------------------
# ---------------------------------------------------

install(TARGETS gocxx
    EXPORT gocxxTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

# For pkg-config
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/gocxx.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/gocxx.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gocxx.pc DESTINATION lib/pkgconfig)

# ---------------------------------------------------
# --- Tests (optional) ------------------------------
# ---------------------------------------------------
# To enable tests, use: cmake -DGOCXX_ENABLE_TESTS=ON ..
# GTest will be automatically fetched if not found locally.
# Tests include:
#   - gocxx_tests: Full test suite using GTest
#   - test_goroutines: Simple standalone test executable

if(GOCXX_ENABLE_TESTS)
    # Find required packages for testing
    find_package(GTest QUIET)
    
    if(NOT GTest_FOUND AND NOT TARGET GTest::gtest)
        message(STATUS "GTest not found, fetching from GitHub...")
        
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.17.0  # Use stable version
            GIT_SHALLOW    TRUE
        )
        
        # Prevent overriding the parent project's compiler/linker settings on Windows
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        
        FetchContent_MakeAvailable(googletest)
        
        # Create aliases to match find_package behavior
        if(NOT TARGET GTest::gtest)
            add_library(GTest::gtest ALIAS gtest)
        endif()
        if(NOT TARGET GTest::gtest_main)
            add_library(GTest::gtest_main ALIAS gtest_main)
        endif()
        
        message(STATUS "GTest fetched and configured")
    else()
        message(STATUS "GTest found locally")
    endif()
    
    # Now we should have GTest available
    if(TARGET GTest::gtest OR TARGET gtest)
        message(STATUS "Building tests with GTest")
        enable_testing()
        
        # Create test executable
        file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
        
        if(TEST_SOURCES)
            add_executable(gocxx_tests ${TEST_SOURCES})
            
            target_link_libraries(gocxx_tests 
                gocxx
                GTest::gtest 
                GTest::gtest_main
            )
            
            target_include_directories(gocxx_tests PRIVATE 
                ${CMAKE_CURRENT_SOURCE_DIR}/include
            )
            
            # Add tests to CTest (simple approach to avoid discovery issues)
            add_test(NAME gocxx_tests COMMAND gocxx_tests)
            
            message(STATUS "Built gocxx tests: ${TEST_SOURCES}")
        else()
            message(WARNING "GOCXX_ENABLE_TESTS is ON but no test files found in tests/")
        endif()
      
        
    else()
        message(FATAL_ERROR "Failed to set up GTest for testing")
    endif()
endif()

# ---------------------------------------------------
# --- Documentation (optional) ----------------------
# ---------------------------------------------------
# To enable documentation, use: cmake -DGOCXX_ENABLE_DOCS=ON ..
# Doxygen will be automatically found and documentation will be generated.

if(GOCXX_ENABLE_DOCS)
    find_package(Doxygen REQUIRED dot)
    
    if(DOXYGEN_FOUND)
        # Configure Doxygen settings
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        # Check if Doxyfile.in exists, if not create a basic one
        if(NOT EXISTS ${DOXYGEN_IN})
            # Create basic Doxyfile from Doxygen
            execute_process(
                COMMAND ${DOXYGEN_EXECUTABLE} -g ${DOXYGEN_IN}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs
                OUTPUT_QUIET
            )
        endif()
        
        # Configure the file
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        # Create docs target
        add_custom_target(docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
        
        # Install documentation
        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html 
                DESTINATION share/doc/gocxx
                OPTIONAL)
        
        message(STATUS "Doxygen documentation will be generated")
    else()
        message(WARNING "Doxygen not found, documentation will not be generated")
    endif()
endif()
