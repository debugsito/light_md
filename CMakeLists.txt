cmake_minimum_required(VERSION 3.16)
project(light_md VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(LIGHT_MD_BUILD_MOBILE "Build the QML mobile UI (desktop preview / future Android)" ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets PrintSupport)
find_package(Qt6 COMPONENTS Test QUIET)
if(LIGHT_MD_BUILD_MOBILE)
    find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)
endif()

qt_standard_project_setup()

add_library(light_md_core STATIC
    src/core/document.cpp
    src/core/document.h
)
target_include_directories(light_md_core PUBLIC src)
target_link_libraries(light_md_core PUBLIC Qt6::Core)
set_target_properties(light_md_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

qt_add_executable(light_md
    src/main.cpp
    src/theme.cpp
    src/theme.h
)

target_link_libraries(light_md PRIVATE light_md_core Qt6::Widgets Qt6::PrintSupport)

set_target_properties(light_md PROPERTIES
    WIN32_EXECUTABLE TRUE
)

install(TARGETS light_md BUNDLE DESTINATION . RUNTIME DESTINATION bin)

if(UNIX AND NOT APPLE)
    install(FILES packaging/linux/light_md.desktop
            DESTINATION share/applications)
    install(FILES packaging/linux/light_md.svg
            DESTINATION share/icons/hicolor/scalable/apps)
endif()

if(WIN32)
    qt_generate_deploy_app_script(
        TARGET light_md
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()

# Core tests are optional so packaging / desktop-only Qt installs can skip them.
include(CTest)
if(BUILD_TESTING AND Qt6Test_FOUND)
    qt_add_executable(light_md_core_tests
        tests/document_tests.cpp
    )
    target_link_libraries(light_md_core_tests PRIVATE light_md_core Qt6::Test)
    add_test(NAME light_md_core_tests COMMAND light_md_core_tests)
endif()

# Mobile / touch UI preview. Same Document core; QML shell for phones later.
if(LIGHT_MD_BUILD_MOBILE)
    qt_policy(SET QTP0001 NEW)
    qt_add_executable(light_md_mobile
        src/mobile/main.cpp
        src/mobile/document_controller.cpp
        src/mobile/document_controller.h
    )
    set_source_files_properties(src/mobile/Main.qml PROPERTIES
        QT_RESOURCE_ALIAS Main.qml
    )
    qt_add_qml_module(light_md_mobile
        URI light_md_mobile
        VERSION 1.0
        QML_FILES src/mobile/Main.qml
        NO_PLUGIN
    )
    target_link_libraries(light_md_mobile PRIVATE
        light_md_core
        Qt6::Quick
        Qt6::QuickControls2
    )
    # qt_add_qml_module creates build/light_md_mobile/ for QML artifacts;
    # keep the binary in build/bin to avoid clashing with that directory.
    set_target_properties(light_md_mobile PROPERTIES
        WIN32_EXECUTABLE TRUE
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()
