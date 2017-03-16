
#include <QUrl>
#include <QDir>

#include "qdockvmap.h"

void QDocKVMapIterator::seekToFirst() {
    this->it = this->kvdb->getDB()->begin();
}

void QDocKVMapIterator::seek(QByteArray key) {
    this->it = this->kvdb->getDB()->find(key);
}

bool QDocKVMapIterator::isValid() {
    return (this->it != this->kvdb->getDB()->end());
}

void QDocKVMapIterator::next() {
    this->it++;
}

QByteArray QDocKVMapIterator::key() {
    return this->it.key();
}

QByteArray QDocKVMapIterator::value() {
    return this->it.value();
}

QDocKVMapIterator::QDocKVMapIterator(QDocKVMap* kvdb) {
    this->kvdb = kvdb;
    this->it = this->kvdb->getDB()->begin();
}

QDocKVMapIterator::~QDocKVMapIterator() {
}

//---

QDocKVInterface* QDocKVMap::newTransaction() {
    return new QDocKVMapTransaction(this->db);
}

int QDocKVMap::writeTransaction() {
    return QDocKVInterface::errorIsNotTransaction;
}

QMap<QByteArray, QByteArray>* QDocKVMap::getDB() {
    return this->db;
}

int QDocKVMap::getValue(QByteArray key, QByteArray &value) {
    QMap<QByteArray, QByteArray>::iterator it = this->db->find(key);
    if (it == this->db->end()) {
        this->setLastError("Not found");
        return QDocKVInterface::errorOperation;
    }
    value = it.value();
    return QDocKVInterface::success;
}

int QDocKVMap::putValue(QByteArray key, QByteArray value) {
    (*this->db)[key] = value;
    return QDocKVInterface::success;
}

int QDocKVMap::deleteValue(QByteArray key) {
    QMap<QByteArray, QByteArray>::iterator it = this->db->find(key);
    if (it == this->db->end()) {
        this->setLastError("Not found");
        return QDocKVInterface::errorOperation;
    }
    this->db->erase(it);
    return QDocKVInterface::success;
}

QDocKVIterator* QDocKVMap::newIterator() {
    return new QDocKVMapIterator(this);
}

QDocKVMap::QDocKVMap(QMap<QByteArray, QByteArray> *db) {
    this->db = db;
    this->valid = true;
}

QDocKVMap::QDocKVMap(QString urlString) {
    this->db = NULL;
    this->valid = false;
    QUrl url(urlString);
    if (!url.isValid()) {
        this->lastError = url.errorString();
        return;
    }
    if (url.scheme() != "map") {
        this->lastError = "Only 'map' scheme was supported";
        return;
    }
    this->db = new QMap<QByteArray, QByteArray>();
    this->valid = (this->db != NULL);
}

QDocKVMap::~QDocKVMap() {
    if (this->valid) {
        delete this->db;
    }
}

//---

int QDocKVMapTransaction::getValue(QByteArray key, QByteArray &value) {
    if ((cacheDeleted.contains(key)) && (cacheDeleted[key])) {
        this->lastError = "Not found";
        return QDocKVInterface::errorOperation;
    }
    if (cache.contains(key)) {
        value = cache[key];
        return QDocKVInterface::success;
    }
    QMap<QByteArray, QByteArray>::iterator it = this->db->find(key);
    if (it == this->db->end()) {
        this->setLastError("Not found");
        return QDocKVInterface::errorOperation;
    }
    value = it.value();
    return QDocKVInterface::success;
}

int QDocKVMapTransaction::putValue(QByteArray key, QByteArray value) {
    this->cache[key] = value;
    this->cacheDeleted[key] = false;
    return QDocKVInterface::success;
}

int QDocKVMapTransaction::deleteValue(QByteArray key) {
    this->cacheDeleted[key] = true;
    return QDocKVInterface::success;
}

QDocKVInterface* QDocKVMapTransaction::newTransaction() {
    return NULL;
}

int QDocKVMapTransaction::writeTransaction() {
    foreach(QByteArray key, this->cacheDeleted.keys()) {
        if (this->cacheDeleted[key]) {
            QMap<QByteArray, QByteArray>::iterator it = this->db->find(key);
            if (it == this->db->end()) {
                this->setLastError("Not found");
                return QDocKVInterface::errorOperation;
            }
            this->db->erase(it);
        }
    }
    foreach(QByteArray key, this->cache.keys()) {
        (*this->db)[key] = this->cache[key];
    }
    return QDocKVInterface::success;
}

QDocKVMapTransaction::QDocKVMapTransaction(QMap<QByteArray, QByteArray>* db) : QDocKVMap(db) {
}

QDocKVMapTransaction::~QDocKVMapTransaction() {
}
