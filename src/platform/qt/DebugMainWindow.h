#pragma once

#include <QWidget>
#include <QPushButton>

#include "Discovery.h"

class DebugMainWindow : public QWidget
{
    Q_OBJECT

public:
    DebugMainWindow(Discovery &discovery, QWidget *parent = nullptr);

private:
    QPushButton *button;
    Discovery &discovery;

private slots:
    void handleButton();
};