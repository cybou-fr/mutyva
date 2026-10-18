cmake_minimum_required(VERSION 3.22)
project(MUTYVA_Demo VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.8 REQUIRED COMPONENTS Quick QuickControls2 Network)
qt_standard_project_setup(REQUIRES 6.8)
qt_add_library(mutyva-core STATIC
    starter/src/DeclarativeRuntime.cpp starter/src/DeclarativeRuntime.h
    starter/src/AppDocument.cpp starter/src/AppDocument.h
    starter/src/DesignerContext.cpp starter/src/DesignerContext.h
    starter/src/InteractionState.cpp starter/src/InteractionState.h
)
target_include_directories(mutyva-core PUBLIC starter/src)
target_link_libraries(mutyva-core PUBLIC Qt6::Core)
qt_add_resources(mutyva-core "runtime_data" PREFIX "/" FILES
    specs/app.schema.json specs/mutation.schema.json
    examples/city-pulse.mutyva.json examples/mutations/add-humidity-card.json
    runtime/component-catalog.json runtime/designer-rules.txt specs/designer-response.schema.json
    specs/app-v1.schema.json specs/mutation-v1.schema.json examples/general-workspace.mutyva.json
)
qt_add_executable(mutyva-demo
    starter/src/main.cpp
    starter/src/WeatherService.cpp
    starter/src/WeatherService.h
)
set_source_files_properties(starter/qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS "Main.qml")
set_source_files_properties(starter/qml/WidgetHost.qml PROPERTIES QT_RESOURCE_ALIAS "WidgetHost.qml")
set_source_files_properties(starter/qml/DesignerPanel.qml PROPERTIES QT_RESOURCE_ALIAS "DesignerPanel.qml")
set_source_files_properties(starter/qml/DesignerContextPanel.qml PROPERTIES QT_RESOURCE_ALIAS "DesignerContextPanel.qml")
set_source_files_properties(starter/qml/GeneralWidget.qml PROPERTIES QT_RESOURCE_ALIAS "GeneralWidget.qml")
qt_add_qml_module(mutyva-demo
    URI Mutyva.Demo
    VERSION 1.0
    QML_FILES starter/qml/Main.qml starter/qml/WidgetHost.qml starter/qml/DesignerPanel.qml starter/qml/DesignerContextPanel.qml starter/qml/GeneralWidget.qml
)
target_link_libraries(mutyva-demo PRIVATE mutyva-core Qt6::Quick Qt6::QuickControls2 Qt6::Network)
if(MSVC)
    target_compile_options(mutyva-core PRIVATE /utf-8)
    target_compile_options(mutyva-demo PRIVATE /utf-8)
endif()
set_target_properties(mutyva-demo PROPERTIES
  MACOSX_BUNDLE TRUE
  WIN32_EXECUTABLE TRUE
)
include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
    qt_add_executable(runtime-tests tests/RuntimeTest.cpp starter/src/WeatherService.cpp starter/src/WeatherService.h)
    target_link_libraries(runtime-tests PRIVATE mutyva-core Qt6::Test Qt6::Quick Qt6::QuickControls2 Qt6::Network)
    target_compile_definitions(runtime-tests PRIVATE MUTYVA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    if(MSVC)
        target_compile_options(runtime-tests PRIVATE /utf-8)
    endif()
    add_test(NAME runtime COMMAND runtime-tests -o "${CMAKE_CURRENT_BINARY_DIR}/runtime-tests.txt,txt" -o -,txt)
    set_tests_properties(runtime PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    set_tests_properties(runtime PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;QT_QUICK_CONTROLS_STYLE=Fusion;QT_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::Core>/../plugins;QML_IMPORT_PATH=$<TARGET_FILE_DIR:Qt6::Core>/../qml")
    if(WIN32)
        set_property(TEST runtime APPEND PROPERTY ENVIRONMENT "QT_QPA_FONTDIR=$ENV{WINDIR}/Fonts")
    endif()
endif()
