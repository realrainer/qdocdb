#ifndef QDOCCOLLECTION_H
#define QDOCCOLLECTION_H

#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QHash>
#include <QList>
#include <QByteArray>

#include "qdockvinterface.h"

#include "qdocutils.h"

typedef struct {
    int id;
    QJsonObject query;
    QJsonArray reply;
    bool triggered;
} td_s_observer;

class QDocCollectionTransaction;

class QDocCollection : public QObject {
    Q_OBJECT

    bool isOpened;
    QString baseDir;
    QDocIdGen* pIdGen;

    void getIndexes();

    QList<QByteArray> findLinkKeys(QJsonObject query, QString curPath, int unionLogic);

    void emitObservers();
    void emitObserver(int observerId);

    QDocCollection(QString collectionDir);

protected:
    QDocKVInterface* kvdb;
    QString lastError;

    QHash<QString, QString> indexes;

    int nextObserverId;
    QHash<int, td_s_observer> observers;

    int find(QJsonObject query, QJsonArray* pReply, QList<QByteArray>& keys);
    int checkValidR(QJsonValue queryPart, QJsonValue docPart, QString curPath, bool& valid);
    int removeByKey(QByteArray linkKey);
    int insert(QJsonObject doc, QString& id, bool overwrite = false);

public:

    enum resultEnum {
        success = 0x0,
        errorQuery = 0x1,
        errorInvalidObject = 0x2,
        errorAlreadyExists = 0x3,
        errorDatabase = 0x4
    };

    QString getLastError();
    QDocCollectionTransaction* newTransaction();

    int addIndexValue(QString indexName, QByteArray value, QString linkKey);
    int addIndexValue(QString indexName, QByteArray value, QList<QString> linkKeyList);
    int removeIndexValue(QString indexName, QByteArray value, QString linkKey);
    int addJsonValueToIndex(QString indexName, QJsonValue jsonValue, QString linkKey);
    int getIndexValueLinkKeys(QString indexName, QJsonValue jsonValue, QJsonArray& linkKeys);

    int getValueByLinkKey(QString linkKey, QJsonValue& value);

    // API
    int set(QJsonObject query, QJsonArray& docs);
    int remove(QJsonObject query);
    int removeId(QString docId);
    int find(QJsonObject query, QJsonArray* pReply);
    int observe(QJsonObject query);
    int unobserve(int);

    int createIndex(QString fieldName, QString indexName);
    // ---

    int printAll();
    QString getBaseDir();

    QDocCollection(QDocKVInterface* kvdb, QString collectionDir, QDocIdGen* pIdGen);
    static QDocCollection* open(QString collectionDir, QDocIdGen* pIdGen);
    static QJsonValue getJsonValueByPath(QJsonValue jsonValue, QString path);
    static QByteArray constructIndexValueKey(QString indexName, QByteArray value);
    static QJsonArray multiply(QJsonArray& a, QJsonArray& b);
    static bool compare(QJsonValue& a, QJsonValue& b, QString oper);
    ~QDocCollection();

signals:
    void observeQueryChanged(int, QJsonArray&);
};

class QDocCollectionTransaction : public QDocCollection {
    QHash<int, td_s_observer>* baseObservers;

public:
    enum resultTransactionEnum {
        errorWriteTransaction = 0x101
    };

    int set(QJsonObject& query, QJsonArray& docs);
    int remove(QJsonObject& query);
    int createIndex(QString fieldName, QString indexName);
    int removeId(QString& docId);

    int writeTransaction();
    QDocCollectionTransaction(QDocKVInterface* kvdb, QString collectionDir, QDocIdGen* pIdGen, QHash<int, td_s_observer>* observers);
};

#endif // QDOCCOLLECTION_H
