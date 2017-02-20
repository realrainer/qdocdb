
#include <QUrl>
#include <QDir>
#include <QHash>

#include "qdockvleveldb.h"

// Opened databases singleton
QHash<QString, QDocKVLevelDB*> dbMap;

//---

void QDocKVLevelDBIterator::seekToFirst() {
    this->it->SeekToFirst();
}

void QDocKVLevelDBIterator::seek(QByteArray key) {
    leveldb::Slice keySlice(key.data(), key.size());
    this->it->Seek(keySlice);
}

bool QDocKVLevelDBIterator::isValid() {
    return this->it->Valid();
}

void QDocKVLevelDBIterator::next() {
    this->it->Next();
}

QByteArray QDocKVLevelDBIterator::key() {
    return QByteArray::fromStdString(this->it->key().ToString());
}

QByteArray QDocKVLevelDBIterator::value() {
    return QByteArray::fromStdString(this->it->value().ToString());
}

QDocKVLevelDBIterator::QDocKVLevelDBIterator(QDocKVLevelDB* kvdb) {
    this->kvdb = kvdb;
    this->it = this->kvdb->getDB()->NewIterator(leveldb::ReadOptions());
}

QDocKVLevelDBIterator::~QDocKVLevelDBIterator() {
    delete this->it;
}

//---

QDocKVInterface* QDocKVLevelDB::newTransaction() {
    return new QDocKVLevelDBTransaction(this->db);
}

int QDocKVLevelDB::writeTransaction() {
    return QDocKVInterface::errorIsNotTransaction;
}

leveldb::DB* QDocKVLevelDB::getDB() {
    return this->db;
}

int QDocKVLevelDB::getValue(QByteArray key, QByteArray &value) {
    leveldb::Slice keySlice(key.data(), key.size());
    std::string valueStr;

    leveldb::Status status = this->db->Get(leveldb::ReadOptions(), keySlice, &valueStr);
    if (!status.ok()) {
        this->setLastError(status);
        return QDocKVInterface::errorOperation;
    }
    value = QByteArray::fromStdString(valueStr);
    return QDocKVInterface::success;
}

int QDocKVLevelDB::putValue(QByteArray key, QByteArray value) {
    leveldb::Slice keySlice(key.data(), key.size());
    leveldb::Slice valueSlice(value.data(), value.size());

    leveldb::WriteOptions options;
    options.sync = true;
    leveldb::Status status = this->db->Put(options, keySlice, valueSlice);
    if (!status.ok()) {
        this->setLastError(status);
        return QDocKVInterface::errorOperation;
    }
    return QDocKVInterface::success;
}

int QDocKVLevelDB::deleteValue(QByteArray key) {
    leveldb::Slice keySlice(key.data(), key.size());
    leveldb::WriteOptions options;
    options.sync = true;
    leveldb::Status status = this->db->Delete(options, keySlice);
    if (!status.ok()) {
        this->setLastError(status);
        return QDocKVInterface::errorOperation;
    }
    return QDocKVInterface::success;
}

QDocKVIterator* QDocKVLevelDB::newIterator() {
    return new QDocKVLevelDBIterator(this);
}

QDocKVLevelDB::QDocKVLevelDB(leveldb::DB *db) {
    this->db = db;
    this->valid = true;
}

QDocKVLevelDB::QDocKVLevelDB(QString urlString) {
    this->db = NULL;
    this->valid = false;
    QUrl url(urlString);
    if (!url.isValid()) {
        this->lastError = url.errorString();
        return;
    }
    if (url.scheme() != "file") {
        this->lastError = "Only 'file' scheme was supported";
        return;
    }
    QDir dir(url.toLocalFile());
    if (!dir.exists()) {
        bool r = dir.mkpath(".");
        if (!r) {
            this->lastError = "Can't create directory " + url.toLocalFile();
            return;
        }
    }
    this->path = dir.canonicalPath();
    if (dbMap.contains(this->path)) {
        this->valid = true;
        this->db = dbMap[this->path]->getDB();
    } else {
        leveldb::Options options;
        options.create_if_missing = true;
        leveldb::Status status = leveldb::DB::Open(options, this->path.toStdString(), &this->db);
        if (status.ok()) {
            this->valid = true;
            dbMap[this->path] = this;
        } else {
            this->setLastError(status);
        }
    }
}

QDocKVLevelDB::~QDocKVLevelDB() {
    if (this->valid) {
        bool find = false;
        foreach (QString path, dbMap.keys()) {
            if ((dbMap[path] != this) && (dbMap[path]->getDB() == this->db)) {
                find = true;
            }
        }
        if (!find) {
            delete this->db;
        }
        dbMap.remove(this->path);
    }
}

//---

int QDocKVLevelDBTransaction::getValue(QByteArray key, QByteArray &value) {
    leveldb::Slice keySlice(key.data(), key.size());
    std::string valueStr;

    if ((cacheDeleted.contains(key)) && (cacheDeleted[key])) {
        this->lastError = "Not found";
        return QDocKVInterface::errorOperation;
    }
    if (cache.contains(key)) {
        value = cache[key];
        return QDocKVInterface::success;
    }

    leveldb::Status status = this->db->Get(leveldb::ReadOptions(), keySlice, &valueStr);
    if (!status.ok()) {
        this->setLastError(status);
        return QDocKVInterface::errorOperation;
    }
    value = QByteArray::fromStdString(valueStr);
    return QDocKVInterface::success;
}

int QDocKVLevelDBTransaction::putValue(QByteArray key, QByteArray value) {
    leveldb::Slice keySlice(key.data(), key.size());
    leveldb::Slice valueSlice(value.data(), value.size());
    this->batch->Put(keySlice, valueSlice);
    this->cache[key] = value;
    this->cacheDeleted[key] = false;
    return QDocKVInterface::success;
}

int QDocKVLevelDBTransaction::deleteValue(QByteArray key) {
    leveldb::Slice keySlice(key.data(), key.size());
    this->batch->Delete(keySlice);
    this->cacheDeleted[key] = true;
    return QDocKVInterface::success;
}

QDocKVInterface* QDocKVLevelDBTransaction::newTransaction() {
    return NULL;
}

int QDocKVLevelDBTransaction::writeTransaction() {
    leveldb::WriteOptions options;
    options.sync = true;
    leveldb::Status status = this->getDB()->Write(options, this->batch);
    if (!status.ok()) {
        this->setLastError(status);
        return QDocKVLevelDBTransaction::errorWriteTransaction;
    }
    return QDocKVInterface::success;
}

QDocKVLevelDBTransaction::QDocKVLevelDBTransaction(leveldb::DB* db) : QDocKVLevelDB(db) {
    this->batch = new leveldb::WriteBatch();
}

QDocKVLevelDBTransaction::~QDocKVLevelDBTransaction() {
    delete this->batch;
}
