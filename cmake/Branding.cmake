# Platform packaging of the supplied MUTYVA identity. Runtime PNGs are in core resources.
if(WIN32 AND NOT ANDROID)
    enable_language(RC)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/branding/platform/windows.rc.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/mutyva-branding.rc" @ONLY)
    target_sources(mutyva-demo PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/mutyva-branding.rc")
elseif(APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(_mutyva_icon "${CMAKE_CURRENT_SOURCE_DIR}/branding/icons/mutyva.icns")
    set_source_files_properties("${_mutyva_icon}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(mutyva-demo PRIVATE "${_mutyva_icon}")
    set_target_properties(mutyva-demo PROPERTIES
        MACOSX_BUNDLE_ICON_FILE "mutyva.icns"
        MACOSX_BUNDLE_BUNDLE_NAME "MUTYVA"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.mutyva.app")
elseif(ANDROID)
    set_target_properties(mutyva-demo PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_PACKAGE_NAME "org.mutyva.app")
elseif(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    install(TARGETS mutyva-demo RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/branding/platform/org.mutyva.app.desktop"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
    foreach(_size 16 24 32 48 64 128 256 512)
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/branding/icons/${_size}x${_size}/mutyva.png"
            DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${_size}x${_size}/apps")
    endforeach()
endif()
