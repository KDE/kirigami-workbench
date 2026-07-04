// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "examplepackage.h"

#include <KPackage/Package>

void ExamplePackageStructure::initPackage(KPackage::Package *package)
{
    package->setDefaultPackageRoot(QStringLiteral("kirigami-workbench/packages/example"));
    package->addDirectoryDefinition("ui", QStringLiteral("contents/ui"));
    package->addFileDefinition("mainscript", QStringLiteral("contents/ui/main.qml"));
    package->setRequired("mainscript", true);
}

K_PLUGIN_CLASS_WITH_JSON(ExamplePackageStructure, "example-packagestructure.json")

#include "examplepackage.moc"
#include "moc_examplepackage.cpp"
