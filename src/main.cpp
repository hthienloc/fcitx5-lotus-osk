#include <QApplication>
#include "osk_controller.h"

int main(int argc, char* argv[]) {
    // Use QApplication since we are using QWidget
    QApplication app(argc, argv);
    app.setApplicationName("Fcitx5 OSK");
    app.setOrganizationName("LotusInputMethod");

    OSKController controller;

    return app.exec();
}
