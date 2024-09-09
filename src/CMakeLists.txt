# SPDX-FileCopyrightText: 2024 Joshua Goins <josh@redstrate.com>
# SPDX-License-Identifier: CC0-1.0

add_executable(kirigami-workbench)
target_sources(kirigami-workbench PRIVATE
        main.cpp
        mainwindow.cpp
        mainwindow.h)
target_include_directories(kirigami-workbench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kirigami-workbench
        PRIVATE
        KF6::I18n
        KF6::XmlGui
        KF6::TextEditor
        Qt6::Core
        Qt6::Widgets
        Qt6::Qml
        Qt6::QuickWidgets)