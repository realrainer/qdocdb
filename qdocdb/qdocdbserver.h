#ifndef QDOCDBSERVER_H
#define QDOCDBSERVER_H


#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QStandardPaths>
#include <QList>
#include <QMap>
#include <QSettings>

#include "qdocdatabase.h"
#include "qdocdblinkbase.h"
#include "qdocutils.h"

class QDocdbServer : public QObject {
    Q_OBJECT

    QTcpServer tcpServer;
    bool valid;
    QString serverName;
    QString baseDir;

    int transactionId;
    int getNextTransactionId();

    QMap<int, QDocCollectionTransaction*> transactions;
    QMap<QString, QDocDatabase*> databases;
    QList<QDocdbLinkBase*> clients;

    QMap<int, QDocdbLinkBase*> transactionClient;
    QMap<int, QDocdbLinkBase*> observeClient;
    QMap<int, QDocCollection*> observeCollection;

    QDocDatabase* getDatabase(QString dbName);

public:
    QDocdbServer(QString serverName, QString baseDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    ~QDocdbServer();

public slots:
    void clientRemoved();
    void newConnection();
    void receive(QDocdbLinkObject*);

    void observeQueryChanged(int observeId, QJsonArray& reply);
};

#endif // QDOCDBSERVER_H
