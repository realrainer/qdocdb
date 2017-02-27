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
    QJsonObject queryOptions;
    QJsonArray reply;
    bool triggered;
} td_s_observer;

class QDocCollectionTransaction;
class QDocCollectionSnapshot;

class QDocCollection : public QObject {
    Q_OBJECT

    bool isOpened;
    QString baseDir;

    int getLastSnapshotId(unsigned char &snapshotId);
    QList<QByteArray> findLinkKeys(QJsonObject query, QString curPath, int unionLogic);
    void emitObserver(int observerId);

    QDocCollection(QString collectionDir);    

public:
    enum classTypeEnum {
        typeCollection = 0x0,
        typeTransaction = 0x1,
        typeReadOnlySnapshot = 0x2
    };

protected:
    classTypeEnum classType;
    unsigned char snapshotId;

    QDocIdGen* pIdGen;
    QDocKVInterface* kvdb;
    QString lastError;

    QHash<QString, QString> getIndexes();

    int nextObserverId;
    QHash<int, td_s_observer> observers;

    inline QByteArray constructIndexValueKey(QString indexName, QByteArray valueData);
    inline QByteArray constructIndexValueKey(QString indexName, QJsonValue jsonValue);
    static inline QByteArray constructIndexValueStartKey(QString indexName, unsigned char snapshotId);

    int getIndexValueLinkKeys(QString indexName, QJsonValue jsonValue, QList<QByteArray>& linkKeys);
    int getValueByLinkKey(QByteArray linkKey, QJsonValue& value);

    int find(QJsonObject query, QJsonArray* pReply, QList<QByteArray>& ids, QJsonObject options = QJsonObject());
    void applyOptions(QJsonArray* pReply, QList<QByteArray>* pIds, QJsonObject options);
    int checkValidR(QJsonValue queryPart, QJsonValue docPart, QString curPath, bool& valid);

    QJsonValue getJsonValue(QByteArray id);
    QJsonValue getJsonValue(QByteArray id, unsigned char& snapshotId, bool& isSingle);
    QJsonValue getJsonValueByLinkKey(QByteArray linkKey);

    int getSnapshotsValue(QList<QString>& snapshots);

public:
    enum resultEnum {
        success = 0x0,
        errorQuery = 0x1,
        errorInvalidObject = 0x2,
        errorAlreadyExists = 0x3,
        errorDatabase = 0x4,
        errorSnapshotIsExists = 0x5,
        errorType = 0x6
    };

    // document iterator
    class iterator {
        QDocKVIterator *kvit;
        unsigned char snapshotId;
        QByteArray beginKey;
        QByteArray prefix;
        void setupPrefix();

    public:
        QByteArray key();
        QJsonValue value();
        QJsonValue value(unsigned char& snapshotId, bool& isSingle);
        void seekToFirst();
        void seek(QByteArray id);
        void next();
        bool isValid();
        iterator(QDocKVIterator* kvit, unsigned char snapshotId);
        ~iterator();
    };
    iterator* newIterator();

    void reloadObservers();
    void emitObservers();

    QString getLastError();
    QString getBaseDir();
    QDocIdGen* getIdGen();
    QDocKVInterface* getKVDB();
    unsigned char getSnapshotId();
    QDocCollection::classTypeEnum getClassType();
    QHash<int, td_s_observer>* getObservers();

    // API
    int find(QJsonObject query, QJsonArray* pReply, QJsonObject options = QJsonObject());
    int count(QJsonObject query, int& replyCount, QJsonObject options = QJsonObject());
    int observe(QJsonObject query, QJsonObject queryOptions);
    int unobserve(int);
    QDocCollectionTransaction* newTransaction();
    int newSnapshot(QString snapshotName);
    QDocCollectionSnapshot* getSnapshot(QString shapshotName);
    int getSnapshotId(QString snapshotName, unsigned char& snapshotId);
    int revertToSnapshot(QString snapshotName);
    int removeSnapshot(QString snapshotName);
    // ---
    // debug
    int printAll();

    static QDocCollection* open(QString collectionDir, QDocIdGen* pIdGen);

    QDocCollection(QDocKVInterface* kvdb, QString collectionDir, QDocIdGen* pIdGen, classTypeEnum classType);
    ~QDocCollection();

signals:
    void observeQueryChanged(int, QJsonArray&);
};

class QDocCollectionTransaction : public QDocCollection {

    QDocCollection* parentColl;
    QHash<int, td_s_observer>* baseObservers;

    int addIndexValue(QString indexName, QJsonValue jsonValue, QByteArray id);
    int addIndexValue(QString indexName, QJsonValue jsonValue, QList<QByteArray> ids);
    int removeIndexValue(QString indexName, QJsonValue jsonValue, QByteArray linkKey);
    int putJsonValue(QByteArray id, QJsonValue jsonValue);

public:
    enum resultTransactionEnum {
        errorWriteTransaction = 0x101
    };

    int putIndexes(QHash<QString, QString>&);
    int putSnapshotsValue(QList<QString>& snapshots);
    int copyIndexValues(unsigned char dstSnapshotId, unsigned char srcSnapshotId);

    // API
    int set(QJsonObject& query, QJsonArray& docs);
    int insert(QJsonObject doc, QByteArray& id, bool overwrite = false);
    int remove(QJsonObject& query);
    int removeById(QByteArray id);
    int createIndex(QString fieldName, QString indexName);
    int writeTransaction();
    // ---

    inline QByteArray constructDocumentLinkKey(QByteArray id);
    static inline QByteArray constructDocumentLinkKey(QByteArray id, unsigned char snapshotId);
    inline QByteArray constructDocumentLinkKey(QString id);
    static inline QByteArray constructDocumentStartKey(QByteArray id);

    QDocCollectionTransaction(QDocCollection* pColl);
};

class QDocCollectionSnapshot : public QDocCollection {
    QDocCollection* parentColl;

public:
    QDocCollectionSnapshot(QDocCollection* pColl, unsigned char);
};

/*
 *  --- Documents (HH - snapshot number, min \x00, max \xFF):
 *  d:DOCUMENT_ID:             -> empty
 *  d:DOCUMENT_ID:\xHH         -> serialized QJsonObject
 *  d:DOCUMENT_ID:\xHH         -> serialized QJsonObject
 *  --- Indexes
 *  in:                        -> serialized QHash<QString, QString> [fieldName] = indexName
 *
 *  iv:INDEX_NAME:\xHH:        -> empty
 *  iv:INDEX_NAME:\xHH:VALUE   -> links to keys in serialized QList<QByteArray>[ "d:DOCUMENT_ID:\xHH", ... ]
 *  --- Snapshots
 *  s:                         -> list of snapshots in serialized QList<QString>
*/


#endif // QDOCCOLLECTION_H
