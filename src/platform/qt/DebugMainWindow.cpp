#include "DebugMainWindow.h"

extern void frame(Discovery const &);

DebugMainWindow::DebugMainWindow(Discovery &discovery, QWidget *parent) :
	discovery(discovery),
  	QWidget(parent)
{
    button = new QPushButton("Run Frame", this);
    button->setGeometry(QRect(QPoint(100, 100), QSize(200, 50)));
    connect(button, &QPushButton::released, this, &DebugMainWindow::handleButton);
}

void DebugMainWindow::handleButton()
{
	frame(discovery);
}