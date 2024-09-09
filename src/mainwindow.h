#pragma once

#include <KXmlGuiWindow>

#include <QQuickWidget>

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    explicit MainWindow();

private:
    QQuickWidget *m_quickWidget = nullptr;
};