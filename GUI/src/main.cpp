#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>
#include <QWindow>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

#include "playerbridge.h"
#include "ffmpegengine.h"
#include "videorenderer.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    QFile bootLog(QCoreApplication::applicationDirPath() + QStringLiteral("/startup.log"));
    if (bootLog.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&bootLog);
        ts << "\n==== START " << QDateTime::currentDateTime().toString(Qt::ISODate) << " ====\n";
    }

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // 注册 VideoRenderer 到 QML
    qmlRegisterType<VideoRenderer>("PlayerGui", 1, 0, "VideoRenderer");
    qmlRegisterUncreatableType<PlayerBridge>("PlayerGui", 1, 0, "PlayerBridge", "Enums only");

    PlayerBridge bridge;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("playerBridge"), &bridge);

    // 将 FFmpegEngine 也暴露给 QML（用于连接 frameReady 信号）
    engine.rootContext()->setContextProperty(QStringLiteral("ffmpegEngine"), bridge.engine());

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [&bootLog]() {
            if (bootLog.isOpen()) {
                QTextStream ts(&bootLog);
                ts << "objectCreationFailed\n";
            }
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &app, [&bootLog](const QList<QQmlError>& warnings) {
        if (!bootLog.isOpen()) {
            return;
        }
        QTextStream ts(&bootLog);
        for (const auto& w : warnings) {
            ts << w.toString() << "\n";
        }
    });

    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    if (auto* window = qobject_cast<QWindow*>(engine.rootObjects().first())) {
        window->show();
        window->raise();
        window->requestActivate();
    }

    auto resolveTestMedia = []() -> QString {
        const QStringList candidates = {
            QDir::current().absoluteFilePath(QStringLiteral("time.mp4")),
            QCoreApplication::applicationDirPath() + QStringLiteral("/time.mp4"),
            QStringLiteral("D:/ffmpegPlayer/GUI/time.mp4"),
            QStringLiteral("D:/qt+ffmpeg+SDL/source/ffmpeg_player/time.mp4")
        };
        for (const QString& p : candidates) {
            if (QFileInfo::exists(p)) {
                return p;
            }
        }
        return QString();
    };

    const QString testMedia = resolveTestMedia();
    if (!testMedia.isEmpty()) {
        QTimer::singleShot(0, &bridge, [testMedia, &bridge]() {
            bridge.openMedia(testMedia);
            bridge.play();
        });
    }

    return app.exec();
}
