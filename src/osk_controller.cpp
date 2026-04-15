#include "osk_controller.h"
#include "osk_window.h"
#include <QDebug>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusMessage>
#include <QApplication>
#include <QAction>
#include <QActionGroup>
#include <QDateTime>
#include <QSettings>
#include <QDir>
#include <QPalette>
#include <QStandardPaths>
#include <sys/socket.h>
#include <poll.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <glob.h>
#include <pwd.h>
#include "lotus-key-command.h"

OSKController::OSKController(QObject* parent) : QObject(parent), m_visible(false), m_window(nullptr), m_socketFd(-1) {
    qDebug() << "Lotus OSK Controller initialized";

    m_hideTimer.setSingleShot(true);
    m_hideTimer.setInterval(250);
    connect(&m_hideTimer, &QTimer::timeout, this, &OSKController::hideWindow);

    m_configPath = QDir::homePath() + "/.config/fcitx5/lotus-osk.conf";

    QDBusConnection::sessionBus().registerService("app.lotus.Osk");
    QDBusConnection::sessionBus().registerObject("/app/lotus/Osk/Controller", this, QDBusConnection::ExportAllSlots);

    // Initialize System Tray Icon
    m_trayIcon = new QSystemTrayIcon(QIcon::fromTheme("input-keyboard"), this);
    m_trayMenu = new QMenu();

    auto* toggleAction = m_trayMenu->addAction("Toggle Keyboard");
    connect(toggleAction, &QAction::triggered, this, &OSKController::Toggle);

    auto* autoShowAction = m_trayMenu->addAction("Enable Auto-Show");
    autoShowAction->setCheckable(true);
    connect(autoShowAction, &QAction::triggered, this, [this](bool checked) {
        m_autoShow = checked;
        saveConfig();
    });

    auto* showEscAction = m_trayMenu->addAction("Show Esc Key");
    showEscAction->setCheckable(true);
    connect(showEscAction, &QAction::triggered, this, [this](bool checked) {
        setShowEsc(checked);
        saveConfig();
    });
    
    auto* autoStartAction = m_trayMenu->addAction("Auto-start with System");
    autoStartAction->setCheckable(true);
    connect(autoStartAction, &QAction::triggered, this, [this](bool checked) {
        setAutoStart(checked);
        saveConfig();
    });

    m_trayMenu->addSeparator();

    // Theme Menu
    QMenu* themeMenu = m_trayMenu->addMenu("Theme");
    QActionGroup* themeGroup = new QActionGroup(this);
    QStringList themes = {"Auto", "Light", "Dark"};
    for (const QString& t : themes) {
        QAction* act = themeMenu->addAction(t);
        act->setCheckable(true);
        themeGroup->addAction(act);
        if (t == m_themeMode) act->setChecked(true);
        connect(act, &QAction::triggered, this, [this, t]() {
            m_themeMode = t;
            saveConfig();
            loadConfig(); // Apply changes
        });
    }

    // Size Menu
    QMenu* sizeMenu = m_trayMenu->addMenu("Size");
    QActionGroup* sizeGroup = new QActionGroup(this);
    QStringList sizes = {"Small", "Standard", "Large"};
    for (const QString& s : sizes) {
        QAction* act = sizeMenu->addAction(s);
        act->setCheckable(true);
        sizeGroup->addAction(act);
        if (s == m_oskSize) act->setChecked(true);
        connect(act, &QAction::triggered, this, [this, s]() {
            m_oskSize = s;
            if (m_window) m_window->setOSKSize(s);
            saveConfig();
        });
    }

    m_trayMenu->addSeparator();

    auto* quitAction = m_trayMenu->addAction("Quit");
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->setToolTip("Lotus OSK");
    m_trayIcon->show();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            Toggle();
        }
    });

    QApplication::setQuitOnLastWindowClosed(false);

    loadConfig();
    
    // Sync Auto-Show checkbox
    autoShowAction->setChecked(m_autoShow);
    // Sync Theme checkboxes
    for (QAction* act : themeGroup->actions()) {
        if (act->text() == m_themeMode) act->setChecked(true);
    }
    // Sync Size checkboxes
    for (QAction* act : sizeGroup->actions()) {
        if (act->text() == m_oskSize) act->setChecked(true);
    }
    // Sync Show Esc checkbox
    showEscAction->setChecked(m_showEsc);
    // Sync Auto-start checkbox
    autoStartAction->setChecked(m_autoStart);
}

void OSKController::loadConfig() {
    QSettings settings(m_configPath, QSettings::IniFormat);
    m_autoShow = settings.value("AutoShow", true).toBool();
    m_themeMode = settings.value("Theme", "Auto").toString();
    m_oskSize = settings.value("Size", "Standard").toString();
    m_showEsc = settings.value("ShowEsc", false).toBool();
    m_autoStart = settings.value("AutoStart", false).toBool();

    if (m_themeMode == "Light") {
        m_whiteTheme = true;
    } else if (m_themeMode == "Dark") {
        m_whiteTheme = false;
    } else {
        m_whiteTheme = !isSystemDarkMode();
    }

    if (m_window) {
        m_window->setWhiteTheme(m_whiteTheme);
        m_window->setOSKSize(m_oskSize);
    }
}

void OSKController::saveConfig() {
    QSettings settings(m_configPath, QSettings::IniFormat);
    settings.setValue("AutoShow", m_autoShow);
    settings.setValue("Theme", m_themeMode);
    settings.setValue("Size", m_oskSize);
    settings.setValue("ShowEsc", m_showEsc);
    settings.setValue("AutoStart", m_autoStart);
    settings.sync();
}

bool OSKController::isSystemDarkMode() const {
    QPalette pal = qApp->palette();
    return pal.color(QPalette::WindowText).lightness() > pal.color(QPalette::Window).lightness();
}

void OSKController::setAutoStart(bool enabled) {
    if (m_autoStart == enabled) return;
    m_autoStart = enabled;
    
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    QDir().mkpath(autostartDir);
    QString desktopPath = autostartDir + "/fcitx5-lotus-osk.desktop";
    
    if (enabled) {
        QFile file(desktopPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n"
                << "Type=Application\n"
                << "Name=Lotus OSK\n"
                << "Comment=On-Screen Keyboard for Lotus Input Method\n"
                << "Exec=fcitx5-lotus-osk\n"
                << "Icon=input-keyboard\n"
                << "Terminal=false\n"
                << "Categories=Settings;Utility;\n"
                << "Keywords=keyboard;osk;lotus;\n"
                << "X-GNOME-Autostart-enabled=true\n";
            file.close();
            qDebug() << "Autostart entry created at" << desktopPath;
        }
    } else {
        if (QFile::exists(desktopPath)) {
            QFile::remove(desktopPath);
            qDebug() << "Autostart entry removed from" << desktopPath;
        }
    }
    emit autoStartChanged();
}

void OSKController::connectToServer() {
    if (m_notifier) {
        m_notifier->setEnabled(false);
        delete m_notifier;
        m_notifier = nullptr;
    }
    if (m_socketFd >= 0)
        close(m_socketFd);

    m_socketFd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);
    if (m_socketFd < 0)
        return;

    QString username = qgetenv("USER");
    if (username.isEmpty())
        username = qgetenv("USERNAME");

    std::string        path = "lotussocket-" + username.toStdString() + "-osk_socket";

    struct sockaddr_un addr{};
    addr.sun_family  = AF_UNIX;

    if (path.length() >= sizeof(addr.sun_path)) {
        qWarning() << "Socket path too long:" << path.c_str();
        close(m_socketFd);
        m_socketFd = -1;
        return;
    }

    addr.sun_path[0] = '\0';
    memcpy(&addr.sun_path[1], path.c_str(), path.length());
    socklen_t len = offsetof(struct sockaddr_un, sun_path) + path.length() + 1;

    if (::connect(m_socketFd, (struct sockaddr*)&addr, len) < 0) {
        close(m_socketFd);
        m_socketFd = -1;
    } else {
        qDebug() << "Connected to lotus-server socket";
        m_notifier = new QSocketNotifier(m_socketFd, QSocketNotifier::Read, this);
        connect(m_notifier, &QSocketNotifier::activated, this, &OSKController::handleSocketActivated);
    }
}

OSKController::~OSKController() {
    if (m_notifier)
        m_notifier->setEnabled(false);
    delete m_notifier;
    if (m_socketFd >= 0)
        close(m_socketFd);
}

void OSKController::setVisible(bool visible) {
    if (visible) {
        m_hideTimer.stop();
        if (!m_visible) {
            showWindow();
        }
    } else {
        if (m_visible && !m_hideTimer.isActive()) {
            m_hideTimer.start();
        }
    }
}

void OSKController::showWindow() {
    if (!m_window) {
        m_window = new OSKWindow(this);
        m_window->setWhiteTheme(m_whiteTheme);
        m_window->setOSKSize(m_oskSize);
    }
    m_window->show();
    m_visible = true;
    notifyServerVisibility();
    emit visibleChanged();
}

void OSKController::hideWindow() {
    if (m_window)
        m_window->hide();
    m_visible = false;
    notifyServerVisibility();
    emit visibleChanged();
}

void OSKController::notifyServerVisibility() {
    if (m_socketFd < 0)
        connectToServer();

    if (m_socketFd >= 0) {
        LotusKeyCommand cmd;
        cmd.type  = m_visible ? LotusKeyCommandType::OskShow : LotusKeyCommandType::OskHide;
        cmd.code  = 0;
        cmd.value = 0;

        if (send(m_socketFd, &cmd, sizeof(cmd), MSG_NOSIGNAL) != sizeof(cmd)) {
            close(m_socketFd);
            m_socketFd = -1;
        }
    }
}

void OSKController::sendKey(bool isRelease, uint keycode) {
    if (m_socketFd < 0)
        connectToServer();

    if (m_socketFd >= 0 && keycode > 0) {
        LotusKeyCommand cmd;
        cmd.type  = LotusKeyCommandType::KeyEvent;
        cmd.code  = keycode;
        cmd.value = isRelease ? 0 : 1;

        if (send(m_socketFd, &cmd, sizeof(cmd), MSG_NOSIGNAL) != sizeof(cmd)) {
            close(m_socketFd);
            m_socketFd = -1;
            if (m_notifier) {
                m_notifier->setEnabled(false);
                delete m_notifier;
                m_notifier = nullptr;
            }
        }
    }
}

static std::vector<std::string> g_capslock_paths;
static void                     refresh_capslock_paths() {
    g_capslock_paths.clear();
    glob_t g;
    if (glob("/sys/class/leds/*capslock/brightness", 0, nullptr, &g) == 0) {
        for (size_t i = 0; i < g.gl_pathc; ++i)
            g_capslock_paths.emplace_back(g.gl_pathv[i]);
        globfree(&g);
    }
}

void OSKController::queryCapsLockState() {
    if (g_capslock_paths.empty())
        refresh_capslock_paths();

    bool anyActive = false;
    for (const auto& path : g_capslock_paths) {
        int fd = open(path.c_str(), O_RDONLY);
        if (fd >= 0) {
            char    buf[16];
            ssize_t r = read(fd, buf, sizeof(buf) - 1);
            close(fd);
            if (r > 0) {
                buf[r] = '\0';
                if (atoi(buf) > 0) {
                    anyActive = true;
                    break;
                }
            }
        }
    }

    if (anyActive != m_capsLockActive) {
        m_capsLockActive = anyActive;
        emit capsLockActiveChanged();
    }
}

void OSKController::handleSocketActivated(int /*socket*/) {
    int     state = 0;
    ssize_t n     = recv(m_socketFd, &state, sizeof(state), 0);
    if (n == sizeof(state)) {
        bool active = state > 0;
        if (active != m_capsLockActive) {
            m_capsLockActive = active;
            emit capsLockActiveChanged();
        }
    } else {
        if (m_notifier) {
            m_notifier->setEnabled(false);
            delete m_notifier;
            m_notifier = nullptr;
        }
        if (m_socketFd >= 0) {
            close(m_socketFd);
            m_socketFd = -1;
        }
        qDebug() << "Server disconnected, cleaned up socket notifier";
    }
}

void OSKController::Show() {
    qDebug() << "DBus Show called, autoShow:" << m_autoShow;
    if (m_autoShow) {
        loadConfig(); // Refresh theme/size on show
        setVisible(true);
    }
}

void OSKController::Hide() {
    qDebug() << "DBus Hide called";
    m_hideTimer.stop();
    hideWindow();
}

void OSKController::Toggle() {
    qDebug() << "DBus Toggle called";
    setVisible(!m_visible);
}

void OSKController::setWhiteTheme(bool white) {
    if (m_whiteTheme != white) {
        m_whiteTheme = white;
        if (m_window) {
            m_window->setWhiteTheme(white);
        }
        emit whiteThemeChanged();
    }
}

void OSKController::SetTheme(bool white) {
    qDebug() << "DBus SetTheme called:" << white;
    setWhiteTheme(white);
}

void OSKController::setOskSize(const QString& size) {
    if (m_oskSize != size) {
        m_oskSize = size;
        if (m_window) {
            m_window->setOSKSize(size);
        }
        emit oskSizeChanged();
    }
}

void OSKController::SetSize(const QString& size) {
    qDebug() << "DBus SetSize called:" << size;
    setOskSize(size);
}

void OSKController::setShowEsc(bool show) {
    if (m_showEsc != show) {
        m_showEsc = show;
        if (m_window) {
            m_window->setupLayout(m_window->currentTheme()); // Refresh layout
        }
        emit showEscChanged();
    }
}
