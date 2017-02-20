#ifndef QDOCKVLEVELDB_H
#define QDOCKVLEVELDB_H

#include <QString>
#include <QHash>

#include "leveldb/leveldb/include/leveldb/db.h"
#include "leveldb/leveldb/include/leveldb/write_batch.h"

#include "qdockvinterface.h"

class QDocKVLevelDB;

class QDocKVLevelDBIterator : public QDocKVIterator {
    leveldb::Iterator* it;
    QDocKVLevelDB* kvdb;

public:
    void seekToFirst();
    void seek(QByteArray key);
    bool isValid();
    void next();
    QByteArray key();
    QByteArray value();
    QDocKVLevelDBIterator(QDocKVLevelDB* kvdb);
    ~QDocKVLevelDBIterator();
};

class QDocKVLevelDB: public QDocKVInterface {

    QString path;

protected:
    leveldb::DB* db;
    void setLastError(leveldb::Status &status) {
        this->lastError = QString::fromStdString(status.ToString());
    }

public:

    enum resultLevelDBEnum {
        errorParseUrl = 0x101
    };

    int putValue(QByteArray key, QByteArray value);
    int getValue(QByteArray key, QByteArray &value);
    int deleteValue(QByteArray key);

    QDocKVInterface* newTransaction();
    int writeTransaction();

    QDocKVIterator* newIterator();

    leveldb::DB* getDB();

    QDocKVLevelDB(leveldb::DB* db);
    QDocKVLevelDB(QString urlString);
    ~QDocKVLevelDB();
};

class QDocKVLevelDBTransaction : public QDocKVLevelDB {
    leveldb::WriteBatch *batch;

    QHash<QByteArray, QByteArray> cache;
    QHash<QByteArray, bool> cacheDeleted;

    enum resultLevelDBTransactionEnum {
        errorWriteTransaction = 0x181
    };

public:
    int putValue(QByteArray key, QByteArray value);
    int getValue(QByteArray key, QByteArray &value);
    int deleteValue(QByteArray key);

    QDocKVInterface* newTransaction();
    int writeTransaction();

    QDocKVLevelDBTransaction(leveldb::DB* db);
    ~QDocKVLevelDBTransaction();
};

#endif // QDOCKVLEVELDB_H
