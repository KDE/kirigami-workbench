# SPDX-FileCopyrightText: 2024 Joshua Goins <josh@redstrate.com>
# SPDX-License-Identifier: CC0-1.0

cmake_minimum_required(VERSION 3.25)
project(KirigamiWorkbench VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_MIN_VERSION 6.5)
set(KF_MIN_VERSION 6.4)

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

find_package(Qt6 ${QT_MIN_VERSION} COMPONENTS Core Widgets Qml QuickWidgets CONFIG REQUIRED)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS CoreAddons XmlGui I18n TextEditor)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEGitCommitHooks)
include(KDEClangFormat)
include(FeatureSummary)

add_subdirectory(src)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
