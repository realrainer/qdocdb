#ifndef SERVICE_H
#define SERVICE_H

#include <QObject>
#include <QTimer>

#include "../qdocdb/qdocdbserver.h"
#include "../qdocdb/qdocdbclient.h"

class MainService : public QObject {
    Q_OBJECT

    QTimer* timer;

    QDocdbCommonConfig* dbconfig;
    QDocdbServer* dbserver;
    QDocdbClient* dbclient;

    QVariantMap doc;
    QString dbUrl;

public:
    MainService();
    ~MainService();

public slots:
    void onTimer();
};

#endif // SERVICE_H
