cmake_minimum_required(VERSION 3.22)

project(SonRapTune VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

option(SONRAPTUNE_BUILD_PLUGIN "Build the JUCE VST3 and Standalone targets" ON)
option(SONRAPTUNE_BUILD_TESTS "Build deterministic P0/E3 tools" ON)

add_library(SonRapTuneCore STATIC
    engine/analysis/YinCandidateDetector.cpp
    engine/tracking/CausalPitchTracker.cpp
    engine/mapping/ScaleMapper.cpp
    engine/correction/CorrectionTrajectory.cpp
    engine/shifting/DualHeadTimeDomainShifter.cpp
    engine/SonRapTuneEngine.cpp
)

target_include_directories(SonRapTuneCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(SonRapTuneCore PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

if(SONRAPTUNE_BUILD_TESTS)
    add_executable(sonraptune_detector_benchmark tools/detector_benchmark.cpp)
    target_link_libraries(sonraptune_detector_benchmark PRIVATE SonRapTuneCore)

    add_executable(sonraptune_core_smoke tools/core_smoke.cpp)
    target_link_libraries(sonraptune_core_smoke PRIVATE SonRapTuneCore)

    add_executable(sonraptune_realtime_contract_smoke tools/realtime_contract_smoke.cpp)
    target_link_libraries(sonraptune_realtime_contract_smoke PRIVATE SonRapTuneCore)

    add_executable(sonraptune_shifter_smoke tools/shifter_smoke.cpp)
    target_link_libraries(sonraptune_shifter_smoke PRIVATE SonRapTuneCore)

    add_test(NAME detector_benchmark COMMAND sonraptune_detector_benchmark)
    add_test(NAME core_smoke COMMAND sonraptune_core_smoke)
    add_test(NAME realtime_contract_smoke COMMAND sonraptune_realtime_contract_smoke)
    add_test(NAME shifter_smoke COMMAND sonraptune_shifter_smoke)
endif()

if(SONRAPTUNE_BUILD_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.14
        GIT_SHALLOW    TRUE
        GIT_SUBMODULES ""
    )
    FetchContent_MakeAvailable(JUCE)

    juce_add_plugin(SonRapTune
        PRODUCT_NAME             "SonRapTune"
        COMPANY_NAME             "MasArray"
        BUNDLE_ID                "com.masarray.sonraptune"
        PLUGIN_MANUFACTURER_CODE MsAr
        PLUGIN_CODE              SrTn
        FORMATS                  VST3 Standalone
        IS_SYNTH                 FALSE
        NEEDS_MIDI_INPUT         TRUE
        NEEDS_MIDI_OUTPUT        FALSE
        IS_MIDI_EFFECT           FALSE
        COPY_PLUGIN_AFTER_BUILD  FALSE
        VST3_CATEGORIES          Fx Pitch Shift
    )

    target_sources(SonRapTune PRIVATE
        src/PluginProcessor.cpp
        src/ParameterLayout.cpp
    )
    target_include_directories(SonRapTune PRIVATE engine src)
    target_compile_definitions(SonRapTune PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        SONRAPTUNE_VERSION_STRING="${PROJECT_VERSION}"
    )
    target_link_libraries(SonRapTune PRIVATE
        SonRapTuneCore
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
    )
endif()
