# SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
# SPDX-License-Identifier: CC0-1.0

kcoreaddons_add_plugin(kirigami_workbench_packagestructure_example
    SOURCES examplepackage.cpp
    INSTALL_NAMESPACE kf6/packagestructure
)
target_link_libraries(kirigami_workbench_packagestructure_example
    KF6::Package
    KF6::I18n
)
