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
    projectgenerator.cpp
    projectgenerator.h
    createprojectdialog.cpp
    createprojectdialog.h
    wbprotocol.h
    pythonrunner.cpp
    pythonrunner.h
    pythonpreviewwidget.cpp
    pythonpreviewwidget.h
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
    KF6::Archive
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickWidgets
  )

install(TARGETS kirigami-workbench ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/../data/workbench_bridge.py"
    DESTINATION "${KDE_INSTALL_DATADIR}/kirigami-workbench"
)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/../data/workbench_bridge.py"
     DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data/kirigami-workbench")

add_subdirectory(packagestructure)

find_path(KDEVAPPWIZARD_TEMPLATES_DIR
    NAMES kde-frameworks6-simple.tar.bz2 kde-frameworks6.tar.bz2
    PATH_SUFFIXES data/kdevappwizard/templates
                  bin/data/kdevappwizard/templates
                  share/kdevappwizard/templates
    DOC "Directory containing kdevappwizard template archives"
)

if(KDEVAPPWIZARD_TEMPLATES_DIR)
    message(STATUS "Found kdevappwizard templates: ${KDEVAPPWIZARD_TEMPLATES_DIR}")

    file(GLOB _template_archives "${KDEVAPPWIZARD_TEMPLATES_DIR}/*.tar.bz2"
                                  "${KDEVAPPWIZARD_TEMPLATES_DIR}/*.zip")
    set(_build_templates_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data/kdevappwizard/templates")
    file(MAKE_DIRECTORY "${_build_templates_dir}")
    foreach(_archive IN LISTS _template_archives)
        get_filename_component(_archive_name "${_archive}" NAME)
        configure_file("${_archive}" "${_build_templates_dir}/${_archive_name}" COPYONLY)
    endforeach()

    install(
        DIRECTORY "${KDEVAPPWIZARD_TEMPLATES_DIR}/"
        DESTINATION "${KDE_INSTALL_KAPPTEMPLATESDIR}"
        FILES_MATCHING PATTERN "*.tar.bz2" PATTERN "*.zip"
    )
else()
    message(WARNING
        "kdevappwizard template archives not found. "
        "Install kapptemplate to enable project scaffolding in Kirigami Workbench. "
        "Set KDEVAPPWIZARD_TEMPLATES_DIR to the directory containing the .tar.bz2 archives."
    )
endif()
