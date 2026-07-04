// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <KPackage/PackageStructure>

class ExamplePackageStructure : public KPackage::PackageStructure
{
    Q_OBJECT
public:
    ExamplePackageStructure(QObject *parent = nullptr, const QVariantList &args = QVariantList())
        : KPackage::PackageStructure(parent, args)
    {
    }
    void initPackage(KPackage::Package *package) override;
};
