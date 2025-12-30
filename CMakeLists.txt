cmake_minimum_required(VERSION 3.16)

option(GAME_ENGINE_DEBUG_MODE "Enable debug mode" ON)
set(GAME_ENGINE_DEFAULT_SHADER_DIR "${CMAKE_BINARY_DIR}/../data/shaders")
set(GAME_ENGINE_DEFAULT_DATA_DIR "${CMAKE_BINARY_DIR}/../data/")

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# prevent installing to system directories. 
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE INTERNAL "")

# Declare the project
project(Game-Engine)


if (APPLE OR EMSCRIPTEN)
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")    # Disable shared builds on platforms where it does not make sense to use them
endif()

set(EXECUTABLE_NAME ${PROJECT_NAME})

# Create an executable or a shared library based on the platform and add our sources to it
if (ANDROID)
	# The SDL java code is hardcoded to load libmain.so on android, so we need to change EXECUTABLE_NAME
	set(EXECUTABLE_NAME main)
	add_library(${EXECUTABLE_NAME} SHARED)
else()
	add_executable(${EXECUTABLE_NAME})
endif()

# Configuration file
configure_file(${CMAKE_SOURCE_DIR}/.editorconfig .editorconfig COPYONLY)

file(GLOB IMGUI_LIB_FILES external/imgui/*)
file( GLOB_RECURSE SOURCE_FILES src/* )
target_sources(${EXECUTABLE_NAME} 
    PRIVATE ${SOURCE_FILES}
    ${IMGUI_LIB_FILES}
    external/imgui/backends/imgui_impl_sdlgpu3.cpp
    external/imgui/backends/imgui_impl_sdl3.cpp
)
target_include_directories( ${EXECUTABLE_NAME}
	PRIVATE ${CMAKE_SOURCE_DIR}/src/
    PUBLIC ${CMAKE_BINARY_DIR}/
    PRIVATE external/soa_vector/
    PRIVATE external/imgui/
    PRIVATE external/stb
)
target_compile_definitions( ${EXECUTABLE_NAME}
	PRIVATE GAME_ENGINE_DEBUG_MODE
    PRIVATE -DGAME_ENGINE_DEFAULT_SHADER_DIR="${GAME_ENGINE_DEFAULT_SHADER_DIR}"
    PRIVATE -DGAME_ENGINE_DEFAULT_DATA_DIR="${GAME_ENGINE_DEFAULT_DATA_DIR}"
)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
# glm
target_compile_definitions(${EXECUTABLE_NAME}
    PRIVATE GLM_ENABLE_EXPERIMENTAL
    PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE
    PRIVATE GLM_FORCE_RADIANS
)
add_subdirectory( external/glm )
add_subdirectory( external/entt )
add_subdirectory( external/fastgltf )

# What is iosLaunchScreen.storyboard? This file describes what Apple's mobile platforms
# should show the user while the application is starting up. If you don't include one,
# then you get placed in a compatibility mode that does not allow HighDPI.
# This file is referenced inside Info.plist.in, where it is marked as the launch screen file.
# It is also ignored on non-Apple platforms. 

# To get an app icon on Apple platforms, add it to your executable.
# Afterward, the image file in Info.plist.in.
if(APPLE)
    target_sources("${EXECUTABLE_NAME}" PRIVATE "src/logo.png")
endif()

target_compile_features(${EXECUTABLE_NAME} PRIVATE cxx_std_20)

# on Web targets, we need CMake to generate a HTML webpage. 
if(EMSCRIPTEN)
	set(CMAKE_EXECUTABLE_SUFFIX ".html" CACHE INTERNAL "")
endif()

# we use EXCLUDE_FROM_ALL so that its install targets and configs don't
# pollute upwards into our configuration.
set(SDL_SHARED ON)
add_subdirectory(external/SDL EXCLUDE_FROM_ALL)

set(SDLTTF_VENDORED ON) # tell SDL_ttf to build its own dependencies
add_subdirectory(external/SDL_ttf EXCLUDE_FROM_ALL)	

set(SPIRV_BUILD_TESTS OFF)
set(SDLSHADERCROSS_VENDORED ON)
add_subdirectory(external/SDL_shadercross EXCLUDE_FROM_ALL)	

set(SDLMIXER_MIDI_NATIVE OFF)     # disable formats we don't use to make the build faster and smaller. Also some of these don't work on all platforms so you'll need to do some experimentation.
set(SDLMIXER_GME OFF)
set(SDLMIXER_WAVPACK OFF)     
set(SDLMIXER_MOD OFF)
set(SDLMIXER_OPUS OFF)
set(SDLMIXER_VENDORED ON)   # tell SDL_mixer to build its own dependencies
add_subdirectory(external/SDL_mixer EXCLUDE_FROM_ALL)

set(SDLIMAGE_VENDORED OFF)
set(SDLIMAGE_AVIF OFF)	# disable formats we don't use to make the build faster and smaller.
set(SDLIMAGE_BMP OFF)
set(SDLIMAGE_WEBP OFF)
add_subdirectory(external/SDL_image EXCLUDE_FROM_ALL)

target_link_libraries(${EXECUTABLE_NAME} PUBLIC 
	SDL3::SDL3
	SDL3_ttf::SDL3_ttf      # remove if you are not using SDL_ttf
    SDL3_mixer::SDL3_mixer  # remove if you are not using SDL_mixer
    SDL3_image::SDL3_image
    SDL3_shadercross::SDL3_shadercross
    glm
    fastgltf
)
target_compile_definitions(${EXECUTABLE_NAME} PUBLIC SDL_MAIN_USE_CALLBACKS)

# Dealing with assets
# We have some non-code resources that our application needs in order to work. How we deal with those differs per platform.
if (APPLE)
    # on Apple targets, the application bundle has a "resources" subfolder where we can place our assets.
    # SDL_GetBasePath() gives us easy access to that location.
    set(input_root "${CMAKE_CURRENT_LIST_DIR}/src")
    macro(add_resource FILE)
        file(RELATIVE_PATH relpath "${input_root}" "${FILE}")
        get_filename_component(relpath "${relpath}" DIRECTORY)
        target_sources(${EXECUTABLE_NAME} PRIVATE ${FILE})
        set_property(SOURCE ${FILE} PROPERTY MACOSX_PACKAGE_LOCATION "Resources/${relpath}")
    endmacro()
    add_resource("${CMAKE_CURRENT_LIST_DIR}/fonts/Inter-VariableFont.ttf")
elseif(EMSCRIPTEN)
    # on the web, we have to put the files inside of the webassembly
    # somewhat unintuitively, this is done via a linker argument.
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE 
        "--preload-file \"${CMAKE_CURRENT_LIST_DIR}/fonts/Inter-VariableFont.ttf@/\""
    )
else()
    if (ANDROID)
        file(MAKE_DIRECTORY "${MOBILE_ASSETS_DIR}")    # we need to create the project Assets dir otherwise CMake won't copy our assets.
    endif()

    # for platforms that do not have a good packaging format, all we can do is copy the assets to the process working directory.
    macro(copy_helper filename)
        if (ANDROID)
            set(outname "${MOBILE_ASSETS_DIR}/${filename}")    # MOBILE_ASSETS_DIR is set in the gradle file
        else()
            set(outname "${CMAKE_BINARY_DIR}/$<CONFIGURATION>/${filename}")
        endif()
        add_custom_command(POST_BUILD
            TARGET "${EXECUTABLE_NAME}"
		    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/fonts/${filename}" "${outname}"
		    DEPENDS "${filename}"
	    )
    endmacro()
    copy_helper("Inter-VariableFont.ttf")
endif()

# set some extra configs for each platform
set_target_properties(${EXECUTABLE_NAME} PROPERTIES 
    # On macOS, make a proper .app bundle instead of a bare executable
    MACOSX_BUNDLE TRUE
    # Set the Info.plist file for Apple Mobile platforms. Without this file, your app
    # will not launch. 
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/Info.plist.in"

    # in Xcode, create a Scheme in the schemes dropdown for the app.
    XCODE_GENERATE_SCHEME TRUE
    # Identification for Xcode
    XCODE_ATTRIBUTE_BUNDLE_IDENTIFIER "com.ravbug.sdl3-sample"
	XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.ravbug.sdl3-sample"
	XCODE_ATTRIBUTE_CURRENTYEAR "${CURRENTYEAR}"
)

# on Visual Studio, set our app as the default project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "${EXECUTABLE_NAME}")

# On macOS Platforms, ensure that the bundle is valid for distribution by calling fixup_bundle.
# note that fixup_bundle does not work on iOS, so you will want to use static libraries 
# or manually copy dylibs and set rpaths
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    # tell Install about the target, otherwise fixup won't know about the transitive dependencies
    install(TARGETS ${EXECUTABLE_NAME}
    	BUNDLE DESTINATION ./install COMPONENT Runtime
   	    RUNTIME DESTINATION ./install/bin COMPONENT Runtime
    )
	
    set(DEP_DIR "${CMAKE_BINARY_DIR}")  # where to look for dependencies when fixing up
    INSTALL(CODE 
        "include(BundleUtilities)
        fixup_bundle(\"$<TARGET_BUNDLE_DIR:${EXECUTABLE_NAME}>\" \"\" \"${DEP_DIR}\")
        " 
    )
    set(CPACK_GENERATOR "DragNDrop")
    include(CPack)
endif()

enable_testing()

file( GLOB_RECURSE TEST_FILES tests/* )
add_executable(
  tests
)
target_sources(tests 
    PRIVATE ${TEST_FILES}
)
target_include_directories(tests 
    PRIVATE ${CMAKE_SOURCE_DIR}/src/
    PUBLIC ${CMAKE_BINARY_DIR}/
)
target_link_libraries(
  tests
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)