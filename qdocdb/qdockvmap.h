#ifndef QDOCKVMAP_H
#define QDOCKVMAP_H

#include <QString>
#include <QHash>
#include <QMap>

#include "qdockvinterface.h"

class QDocKVMap;

class QDocKVMapIterator : public QDocKVIterator {
    QMap<QByteArray, QByteArray>::iterator it;
    QDocKVMap* kvdb;

public:
    void seekToFirst();
    void seek(QByteArray key);
    bool isValid();
    void next();
    QByteArray key();
    QByteArray value();
    QDocKVMapIterator(QDocKVMap* kvdb);
    ~QDocKVMapIterator();
};

class QDocKVMap: public QDocKVInterface {

    QString path;

protected:
    QMap<QByteArray, QByteArray>* db;
    void setLastError(QString lastError) {
        this->lastError = lastError;
    }

public:

    enum resultMapEnum {
        errorParseUrl = 0x101
    };

    int putValue(QByteArray key, QByteArray value);
    int getValue(QByteArray key, QByteArray &value);
    int deleteValue(QByteArray key);

    QDocKVInterface* newTransaction();
    int writeTransaction();

    QDocKVIterator* newIterator();

    QMap<QByteArray, QByteArray>* getDB();

    QDocKVMap(QMap<QByteArray, QByteArray>* db);
    QDocKVMap(QString urlString);
    ~QDocKVMap();
};

class QDocKVMapTransaction : public QDocKVMap {

    QHash<QByteArray, QByteArray> cache;
    QHash<QByteArray, bool> cacheDeleted;

    enum resultMapTransactionEnum {
        errorWriteTransaction = 0x181
    };

public:
    int putValue(QByteArray key, QByteArray value);
    int getValue(QByteArray key, QByteArray &value);
    int deleteValue(QByteArray key);

    QDocKVInterface* newTransaction();
    int writeTransaction();

    QDocKVMapTransaction(QMap<QByteArray, QByteArray>* db);
    ~QDocKVMapTransaction();
};


#endif // QDOCKVMAP_H
