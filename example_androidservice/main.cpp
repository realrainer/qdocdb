
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>

#ifdef Q_OS_ANDROID
#include <QAndroidJniObject>
#include <QtAndroid>
#endif

#include "../qdocdb/qdocdbconnector.h"
#include "service.h"

int main(int argc, char *argv[]) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

#ifdef Q_OS_ANDROID
    if (argc > 1){
        QCoreApplication app(argc, argv);
        qInfo() << "Starting SERVICE" << endl;
        MainService* mainService = new MainService();
        int r = app.exec();
        delete mainService;
        return r;
    } else {
        QGuiApplication app(argc, argv);
        qInfo() << "Starting APPLICATION" << endl;
        qmlRegisterType<QDocdbConnector>("com.example.qdocdb.classes", 1, 0, "QDocdbConnector");
        QQmlApplicationEngine engine;
        engine.load(QUrl(QLatin1String("qrc:/main.qml")));
        int r = app.exec();
        return r;
    }
#else
    QGuiApplication app(argc, argv);
    MainService* mainService = new MainService();
    qmlRegisterType<QDocdbConnector>("com.example.qdocdb.classes", 1, 0, "QDocdbConnector");
    QQmlApplicationEngine engine;
    engine.load(QUrl(QLatin1String("qrc:/main.qml")));
    int r = app.exec();
    delete mainService;
    return r;
#endif
}
