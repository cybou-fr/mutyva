cmake_minimum_required(VERSION 3.22)
project(MUTYVA_Demo VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.8 REQUIRED COMPONENTS Quick QuickControls2 Network)
qt_standard_project_setup(REQUIRES 6.8)
qt_add_executable(mutyva-demo
    starter/src/main.cpp
    starter/src/WeatherService.cpp
    starter/src/WeatherService.h
    starter/src/AppDocument.cpp
    starter/src/AppDocument.h
)
set_source_files_properties(starter/qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS "Main.qml")
set_source_files_properties(starter/qml/WidgetHost.qml PROPERTIES QT_RESOURCE_ALIAS "WidgetHost.qml")
qt_add_qml_module(mutyva-demo
    URI Mutyva.Demo
    VERSION 1.0
    QML_FILES starter/qml/Main.qml starter/qml/WidgetHost.qml
)
qt_add_resources(mutyva-demo "demo_data" PREFIX "/" FILES examples/city-pulse.mutyva.json)
target_link_libraries(mutyva-demo PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Network)
set_target_properties(mutyva-demo PROPERTIES
  MACOSX_BUNDLE TRUE
  WIN32_EXECUTABLE TRUE
)
