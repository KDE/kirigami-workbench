# SPDX-FileCopyrightText: 2024 Joshua Goins <josh@redstrate.com>
# SPDX-License-Identifier: CC0-1.0

add_executable(kirigami-workbench
    main.cpp
    mainwindow.cpp
    mainwindow.h
    workbenchtabbar.cpp
    workbenchtabbar.h
    workbenchviewspace.cpp
    workbenchviewspace.h
    componentproject.cpp
    componentproject.h
    tabpage.cpp
    tabpage.h
    tabmanager.cpp
    tabmanager.h
    examplemodel.cpp
    examplemodel.h
    gallerypanel.cpp
    gallerypanel.h
    welcomepage.cpp
    welcomepage.h
)

qt_add_resources(kirigami-workbench "assets"
   FILES assets/kirigami-horizontal.png
)

ki18n_install(po)

install(FILES kirigami-workbenchui.rc DESTINATION ${KDE_INSTALL_KXMLGUIDIR})

configure_file(kirigami-workbenchui.rc ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/kirigami-workbenchui.rc COPYONLY)

target_include_directories(kirigami-workbench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kirigami-workbench PRIVATE
    KF6::I18n
    KF6::IconThemes
    KF6::XmlGui
    KF6::TextEditor
    KF6::Package
    Qt6::Core
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickWidgets
  )

install(TARGETS kirigami-workbench ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(packagestructure)
