cmake_minimum_required(VERSION 3.16)

project(deepin-ventoy-installer
    VERSION 1.1.0
    DESCRIPTION "A Ventoy installer front-end built with DTK"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Dtk6 REQUIRED COMPONENTS Widget)

set(SOURCES
    main.cpp
    mainwindow.cpp
    widget.cpp
    dvidroparea.cpp
    utilities.cpp
)

set(HEADERS
    mainwindow.h
    widget.h
    dvidroparea.h
    utilities.h
)

set(UI_FILES
    widget.ui
)

set(RESOURCES
    res.qrc
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${RESOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Dtk6::Widget
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    QT_DEPRECATED_WARNINGS
    VERSION="${PROJECT_VERSION}"
)

# Translations
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

set(TS_FILES
    translations/dvtoyinst_zh.ts
    translations/dvtoyinst_es.ts
    translations/dvtoyinst_pt.ts
    translations/dvtoyinst_it.ts
    translations/dvtoyinst_fr.ts
)

qt_add_translations(${PROJECT_NAME}
    TS_FILES ${TS_FILES}
    SOURCES ${SOURCES} ${HEADERS}
)

# Installation
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES dvtoyinst.desktop DESTINATION share/applications)
install(FILES res/dvtoyinst.png DESTINATION share/icons/hicolor/512x512/apps)

foreach(_lang zh es pt it fr)
    install(FILES "translations/dvtoyinst_${_lang}.qm"
            DESTINATION share/deepin-ventoy-installer/translations
            OPTIONAL)
endforeach()
