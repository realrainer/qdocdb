#ifndef QDOCDBCLIENT_H
#define QDOCDBCLIENT_H

#include <QVariant>
#include <QJsonArray>
#include <QMap>
#include <QTimer>

#include "qdocdblinkbase.h"

class QDocdbClient: public QObject {
    Q_OBJECT

    QString serverName;
    QDocdbLinkBase* client;
    int nextFId;

    QMap<int, QDocdbLinkObject*> activeQueries;
    QMap<int, QDocdbLinkObject*> replies;
    QMap<int, QObject*> subs;

    typedef struct {
        QObject* sub;
        QString url;
        QVariantMap query;
        QVariantMap queryOptions;
    } ObserverInfo;

    QMap<int, QDocdbClient::ObserverInfo> observerInfo;

    typedef struct {
        int observeId;
        QJsonArray reply;
    } ObserveData;

    QQueue<ObserveData> observeData;
    bool observeDataLock;

    QString lastError;

    QTimer connectTimer;
    int nextConnectWaitTimeout;
    bool needConnected;

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

    int insert(QString url, QVariantMap doc, QByteArray& docId, bool overwrite = false, bool ignoreReadOnlyValue = false, int transactionId = -1);
    int remove(QString url, QVariantMap query, int transactionId = -1);
    int removeId(QString url, QByteArray docId, bool ignoreReadOnlyValue = false, int transactionId = -1);

    int set(QString url, QVariantMap query, QVariantList docs, int transactionId = -1);

    int newSnapshot(QString url, QString snapshot);
    int revertToSnapshot(QString url, QString snapshot);
    int removeSnapshot(QString url, QString snapshot);
    int getModified(QString url, QVariantList& docIds, QString snapshot = "__CURRENT");
    int getSnapshotList(QString url, QStringList& snapshotList);

    int observe(QObject* sub, QString url, QVariantMap query, QVariantMap queryOptions = QVariantMap(), int preferedId = -1);
    int unobserve(int observeId);

    int exclusiveLock(QString url);
    int unlock(QString url);

    int connectToServer(QString serverName);
    void disconnectFromServer();
    bool isConnected();

    void cancelAllActiveQueries();

    int newLinkObject(QDocdbLinkObject::LinkObjectType, QDocdbLinkObject**);

    QDocdbClient();
    ~QDocdbClient();

signals:
    void observeReplyWaitDone();

public slots:
    void clientRemoved();
    void receive(QDocdbLinkObject*);
    void destroyReply(int id);
    void subscriberDestroyed(QObject*);

    void observeDataProcess();

    void onConnectTimer();
};

#endif // QDOCDBCLIENT_H
