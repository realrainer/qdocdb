#ifndef QDOCDBCLIENT_H
#define QDOCDBCLIENT_H

#include <QVariant>
#include <QJsonArray>
#include <QMap>

#include "qdocdblinkbase.h"

class QDocdbClient: public QObject {
    Q_OBJECT

    QDocdbLinkBase* client;
    int nextFId;

    QMap<int, QDocdbLinkObject*> activeQueries;
    QMap<int, QDocdbLinkObject*> replies;
    QMap<int, QObject*> subs;

    QString lastError;

    int sendAndWaitReply(int id, QDocdbLinkObject*);

public:
    enum resultEnum {
        success = 0x0,
        errorConnection = 0x1,
        error = 0x2
    };

    QString getLastError() {
        return this->lastError;
    }

    int find(QString url,
             QVariantMap query, QVariantList &reply, QVariantMap queryOptions = QVariantMap(),
             QString snapshot = "__CURRENT");
    int findOne(QString url,
             QVariantMap query, QVariantMap &reply, QVariantMap queryOptions = QVariantMap(),
             QString snapshot = "__CURRENT");
    int count(QString url,
             QVariantMap query, int &reply, QVariantMap queryOptions = QVariantMap(),
             QString snapshot = "__CURRENT");
    int createIndex(QString url, QString fieldName, QString indexName);

    int newTransaction(QString url);
    int writeTransaction(int transactionId);
    int discardTransaction(int transactionId);

    int insert(QString url, QVariantMap doc, QByteArray& docId, bool overwrite = false, int transactionId = -1);
    int remove(QString url, QVariantMap query, int transactionId = -1);
    int removeId(QString url, QByteArray docId, int transactionId = -1);

    int set(QString url, QVariantMap query, QVariantList docs, int transactionId = -1);

    int newSnapshot(QString url, QString snapshot);
    int revertToSnapshot(QString url, QString snapshot);
    int removeSnapshot(QString url, QString snapshot);

    int observe(QObject* sub, QString url, QVariantMap query, QVariantMap queryOptions = QVariantMap());
    int unobserve(QString url, int observeId);

    int connectToServer(QString serverName);
    void disconnectFromServer();
    bool isConnected();

    int newLinkObject(QDocdbLinkObject::LinkObjectType, QDocdbLinkObject**);

    QDocdbClient();
    ~QDocdbClient();

public slots:
    void clientRemoved();
    void receive(QDocdbLinkObject*);
    void destroyReply(int id);

    void observeQueryChanged(int observeId, QJsonArray reply);
};

#endif // QDOCDBCLIENT_H
