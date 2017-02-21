
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>
#include <QDataStream>
#include <QList>

#include "qdocutils.h"
#include "qdockvleveldb.h"
#include "qdoccollection.h"

//---

QString QDocCollection::getLastError() {
    return this->lastError;
}

//---

QByteArray QDocCollection::constructIndexValueKey(QString indexName, QJsonValue jsonValue) {
    QByteArray key = QString("iv:" + indexName + ":").toLocal8Bit();
    key.append(QDocSerialize::marshal(jsonValue));
    return key;
}

QJsonValue QDocCollection::getJsonValue(QByteArray key) {
    QByteArray valueData;
    int r = this->kvdb->getValue(key, valueData);
    if (r != QDocCollection::success) {
        return QJsonValue(QJsonValue::Undefined);
    }
    return QDocSerialize::unmarshal(valueData);
}

QJsonValue QDocCollection::getJsonValue(QDocKVIterator* it) {
    QByteArray valueData = it->value();
    return QDocSerialize::unmarshal(valueData);
}

int QDocCollection::putJsonValue(QByteArray key, QJsonValue jsonValue) {
    int r = this->kvdb->putValue(key, QDocSerialize::marshal(jsonValue));
    if (r != QDocKVInterface::success) {
        this->lastError = this->kvdb->getLastError();
        return QDocCollection::errorDatabase;
    }
    return QDocCollection::success;
}

int QDocCollection::createIndex(QString fieldName, QString indexName) {
    QDocCollectionTransaction* tx = this->newTransaction();
    int r = tx->createIndex(fieldName, indexName);
    if (r != QDocCollection::success) {
        this->lastError = tx->getLastError();
        delete tx;
        return r;
    }
    r = tx->writeTransaction();
    if (r != QDocCollection::success) {
        this->lastError = tx->getLastError();
        delete tx;
        return r;
    }
    delete tx;
    return QDocCollection::success;
}

int QDocCollection::addIndexValue(QString indexName, QJsonValue jsonValue, QString linkKey) {
    QByteArray key = constructIndexValueKey(indexName, jsonValue);
    QJsonArray jsonArray;
    jsonValue = this->getJsonValue(key);
    if (jsonValue.isArray()) {
        jsonArray = jsonValue.toArray();
    }

    QJsonArray::iterator it;
    for (it = jsonArray.begin(); it != jsonArray.end(); it++) {
        if (it->isString()) {
            if (it->toString() == linkKey) {
                return QDocCollection::success;
            }
        }
    }
    jsonArray.append(QJsonValue(linkKey));
    return this->putJsonValue(key, jsonArray);
}

int QDocCollection::getIndexValueLinkKeys(QString indexName, QJsonValue jsonValue, QJsonArray& linkKeys) {
    QByteArray key = constructIndexValueKey(indexName, jsonValue);
    jsonValue = this->getJsonValue(key);
    if (jsonValue.isArray()) {
        linkKeys = jsonValue.toArray();
        return QDocCollection::success;
    } else {
        return QDocCollection::errorDatabase;
    }
}

int QDocCollection::removeIndexValue(QString indexName, QJsonValue jsonValue, QString linkKey) {
    QByteArray key = constructIndexValueKey(indexName, jsonValue);
    QJsonArray jsonArray;
    jsonValue = this->getJsonValue(key);
    if (jsonValue.isArray()) {
        jsonArray = jsonValue.toArray();
    }
    int findIndex = -1;
    for (int i = 0; i < jsonArray.size(); (i++) && (findIndex == -1)) {
        jsonValue = jsonArray[i];
        if (jsonValue.isString()) {
            if (jsonValue.toString() == linkKey) {
                findIndex = i;
            }
        }
    }
    if (findIndex == -1) {
        return QDocCollection::success;
    }
    jsonArray.removeAt(findIndex);
    if (jsonArray.size()) {
        return this->putJsonValue(key, jsonArray);
    } else {
        return this->kvdb->deleteValue(key);
    }
}

int QDocCollection::addIndexValue(QString indexName, QJsonValue jsonValue, QList<QString> linkKeyList) {
    foreach(QString linkKey, linkKeyList) {
        int r = this->addIndexValue(indexName, jsonValue, linkKey);
        if (r != QDocCollection::success) {
            return r;
        }
    }
    return QDocCollection::success;
}

int QDocCollection::addJsonValueToIndex(QString indexName, QJsonValue jsonValue, QString linkKey) {
    int r;
    if (jsonValue.isArray()) {
        QJsonArray jsonArray = jsonValue.toArray();
        QJsonArray::iterator it;
        for (it = jsonArray.begin(); it != jsonArray.end(); it++) {
            r = this->addIndexValue(indexName, *it, linkKey);
            if (r != QDocCollection::success) {
                return r;
            }
        }
    }
    r = this->addIndexValue(indexName, jsonValue, linkKey);
    if (r != QDocCollection::success) {
        return r;
    }
    return QDocCollection::success;
}

//---

void QDocCollection::getIndexes() {
    QDocKVIterator* it = this->kvdb->newIterator();
    for (it->seek("in:"); it->isValid() && it->key().mid(0, 3) == "in:"; it->next()) {
        QString fieldName = it->key().mid(3);
        if (!fieldName.isEmpty()) {
            this->indexes[fieldName] = it->value();
        }
    }
    delete it;
}

//---

void QDocCollection::emitObserver(int observerId) {
    if (this->observers[observerId].triggered) {
        this->observers[observerId].triggered = false;
        emit this->observeQueryChanged(observerId, this->observers[observerId].reply);
    }
}

void QDocCollection::emitObservers() {
    foreach(int observerId, this->observers.keys()) {
        this->emitObserver(observerId);
    }
}

int QDocCollection::checkValidR(QJsonValue queryPart, QJsonValue docPart, QString curPath, bool &valid) {
    if (!queryPart.isObject()) return QDocCollection::success;
    int r;

    QJsonObject queryPartObj = queryPart.toObject();
    valid = true;
    foreach(QString key, queryPartObj.keys()) {
        bool valid1 = true;
        QJsonValue queryValue = queryPartObj[key];

        if (key[0] == '$') {
            if (key == "$exists") {
                if (queryValue.isBool()) {
                    valid1 &= !((queryValue.toBool()) == (!(docPart != QJsonValue(QJsonValue::Undefined))));
                } else {
                    return QDocCollection::errorQuery;
                }
            } else
            if ((key == "$gt") || (key == "$gte") || (key == "$lt") || (key == "$lte")) {
                valid1 &= QDocUtils::compare(docPart, queryValue, key);
            } else
            if (key == "$in") {
                if (queryValue.isArray()) {
                    QJsonArray queryArray = queryValue.toArray();
                    bool find = false;
                    for (QJsonArray::iterator it = queryArray.begin(); (it != queryArray.end()) && (!find); it++) {
                        if (docPart == *it) find = true;
                    }
                    valid1 &= find;
                } else return QDocCollection::errorQuery;
            } else
            if (key == "$or") {
                if (queryValue.isArray()) {
                    valid1 = false;
                    QJsonArray queryArray = queryValue.toArray();
                    for (QJsonArray::iterator it = queryArray.begin(); (it != queryArray.end()) && (!valid1); it++) {
                        bool valid2;
                        r = this->checkValidR(*it, docPart, curPath, valid2);
                        if (r != QDocCollection::success) {
                            return r;
                        }
                        valid1 |= valid2;
                    }
                } else return QDocCollection::errorQuery;
            } else
            if (key == "$and") {
                if (queryValue.isArray()) {
                    valid1 = true;
                    QJsonArray queryArray = queryValue.toArray();
                    for (QJsonArray::iterator it = queryArray.begin(); (it != queryArray.end()) && (valid1); it++) {
                        bool valid2;
                        r = this->checkValidR(*it, docPart, curPath, valid2);
                        if (r != QDocCollection::success) {
                            return r;
                        }
                        valid1 &= valid2;
                    }
                } else return QDocCollection::errorQuery;
            } else {
                return QDocCollection::errorQuery;
            }
        } else {
            QString valuePath;
            if (curPath == "") {
                valuePath = key;
            } else {
                valuePath = curPath + "." + key;
            }
            QJsonValue docValue = QDocUtils::getJsonValueByPath(docPart, key);

            if (queryValue.isObject()) {
                r = this->checkValidR(queryValue, docValue, valuePath, valid1);
                if (r != QDocCollection::success) {
                    return r;
                }
            } else {
                if (!docValue.isArray()) {
                    if (docValue != queryValue) valid1 = false;
                } else {
                    bool find = false;
                    QJsonArray docArray = docValue.toArray();
                    for (QJsonArray::iterator it = docArray.begin(); (it != docArray.end()) && (!find); it++) {
                        if (*it == queryValue) find = true;
                    }
                    if (!find) valid1 = false;
                }
            }
        }
        valid &= valid1;
    }
    return QDocCollection::success;
}


int QDocCollection::insert(QJsonObject doc, QString &id, bool overwrite) {
    QJsonObject::iterator i_id = doc.find("_id");
    if (i_id == doc.end()) {
        id = this->pIdGen->getId();
        doc.insert("_id", QJsonValue(id));
    } else {
        if (i_id.value().isString()) {
            id = i_id.value().toString();
        } else {
            this->lastError = "_id value is not string";
            return QDocCollection::errorInvalidObject;
        }
    }
    QByteArray key = QString("d:" + id).toLocal8Bit();
    QByteArray value;
    bool exists = false;
    if (this->kvdb->getValue(key, value) == QDocCollection::success) {
        exists = true;
        if (!overwrite) {
            return QDocCollection::errorAlreadyExists;
        }
    }
    int r = this->putJsonValue(key, QJsonValue(doc));
    if (r != QDocCollection::success) {
        return r;
    }

    foreach (QString fieldName, indexes.keys()) {
        QJsonValue jsonValue = QDocUtils::getJsonValueByPath(QJsonValue(doc), fieldName);
        QString indexName = indexes[fieldName];
        r = this->addJsonValueToIndex(indexName, jsonValue, key);
        if (r != QDocCollection::success) {
            return r;
        }
    }

    foreach(int observeId, this->observers.keys()) {
        bool valid = false;
        int r = this->checkValidR(this->observers[observeId].query, QJsonValue(doc), "", valid);
        if ((r == QDocCollection::success) && (valid)) {
            QJsonArray& reply = this->observers[observeId].reply;
            if (!exists) {
                reply.append(QJsonValue(doc));
            } else {
                QJsonArray::iterator it = reply.begin();
                bool changed = false;
                while ((it != reply.end()) && (!changed)) {
                    if (doc.value("_id") == it->toObject().value("_id")) {
                        *it = QJsonValue(doc);
                        changed = true;
                    }
                    it++;
                }
            }
            this->observers[observeId].triggered = true;
        }
    }

    return QDocCollection::success;
}

//---

int QDocCollection::set(QJsonObject query, QJsonArray& docs) {
    QDocCollectionTransaction* tx = this->newTransaction();
    int r = tx->set(query, docs);
    if (r != QDocCollection::success) {
        delete tx;
        return r;
    }
    r = tx->writeTransaction();
    if (r != QDocCollection::success) {
        delete tx;
        return r;
    }
    delete tx;
    this->emitObservers();
    return QDocCollection::success;
}

int QDocCollection::removeByKey(QByteArray linkKey) {
    QJsonValue jsonValue = this->getJsonValue(linkKey);
    if (!jsonValue.isObject()) {
        this->lastError = "value is not object";
        return QDocCollection::errorDatabase;
    }
    QJsonObject doc = jsonValue.toObject();
    foreach (QString fieldName, indexes.keys()) {
        QJsonValue jsonValue = QDocUtils::getJsonValueByPath(QJsonValue(doc), fieldName);
        QString indexName = indexes[fieldName];
        if (jsonValue.isArray()) {
            QJsonArray jsonArray = jsonValue.toArray();
            QJsonArray::iterator it;
            for (it = jsonArray.begin(); it != jsonArray.end(); it++) {
                int r = this->removeIndexValue(indexName, *it, linkKey);
                if (r != QDocCollection::success) {
                    return r;
                }
            }
        }
        int r = this->removeIndexValue(indexName, jsonValue, linkKey);
        if (r != QDocCollection::success) {
            return r;
        }
    }
    int r = this->kvdb->deleteValue(linkKey);
    if (r != QDocKVInterface::success) {
        return r;
    }

    foreach(int observeId, this->observers.keys()) {
        bool valid = false;
        int r = this->checkValidR(this->observers[observeId].query, jsonValue, "", valid);
        QJsonArray& reply = this->observers[observeId].reply;
        if ((r == QDocCollection::success) && (valid)) {
            QJsonArray::iterator it = reply.begin();
            bool deleted = false;
            while ((it != reply.end()) && (!deleted)) {
                if (doc.value("_id") == it->toObject().value("_id")) {
                    reply.erase(it);
                    deleted = true;
                    this->observers[observeId].triggered = true;
                }
                it++;
            }
        }
    }

    return QDocCollection::success;
}

int QDocCollection::remove(QJsonObject query) {

    QDocCollectionTransaction* tx = this->newTransaction();
    int r = tx->remove(query);
    if (r != QDocCollection::success) {
        this->lastError = tx->getLastError();
        delete tx;
        return r;
    }
    r = tx->writeTransaction();
    if (r != QDocCollection::success) {
        this->lastError = tx->getLastError();
        delete tx;
        return r;
    }
    delete tx;
    this->emitObservers();

    return QDocCollection::success;
}

int QDocCollection::removeId(QString docId) {
    QDocCollectionTransaction* tx = this->newTransaction();
    int r = tx->removeId(docId);
    if (r != QDocCollection::success) {
        this->lastError = tx->getLastError();
        delete tx;
        return r;
    }
    r = tx->writeTransaction();
    if (r != QDocCollection::success) {
        this->lastError = tx->getLastError();
        delete tx;
        return r;
    }
    delete tx;
    this->emitObservers();
    return QDocCollection::success;
}

int QDocCollection::find(QJsonObject query, QJsonArray* pReply, QList<QByteArray>& keys) {

    keys = QList<QByteArray>();
    *pReply = QJsonArray();

    // try find indexes
    bool firstFind = true;
    QJsonArray linkKeys;

    foreach(QString key, query.keys()) {
        if ((indexes.contains(key)) && (!query[key].isArray()) && (!query[key].isObject())) {
            QJsonArray indexLinkKeys;
            this->getIndexValueLinkKeys(indexes[key], query[key], indexLinkKeys);
            if (firstFind) {
                firstFind = false;
                linkKeys = indexLinkKeys;
            } else {
                linkKeys = QDocUtils::multiply(linkKeys, indexLinkKeys);
            }
        }
    }
    if (!firstFind) {
        for (QJsonArray::iterator it = linkKeys.begin(); it != linkKeys.end(); it++) {
            QByteArray linkKey = (*it).toString().toLocal8Bit();
            QJsonValue jsonValue = this->getJsonValue(linkKey);
            if (jsonValue.isObject()) {
                bool valid = true;
                int r = this->checkValidR(query, jsonValue, "", valid);
                if (r != QDocCollection::success) {
                    return r;
                }
                if (valid) {
                    pReply->append(jsonValue);
                    keys.append(linkKey);
                }
            }
        }
        return QDocCollection::success;
    }

    // full search
    QDocKVIterator* it = this->kvdb->newIterator();
    for (it->seekToFirst(); it->isValid() && it->key().mid(0, 2) == "d:"; it->next()) {
        QByteArray docKey = it->key();
        QJsonValue jsonValue = this->getJsonValue(it);
        if (jsonValue.isObject()) {
            bool valid = true;
            int r = this->checkValidR(query, jsonValue, "", valid);
            if (r != QDocCollection::success) {
                return r;
            }
            if (valid) {
                pReply->append(jsonValue);
                keys.append(docKey);
            }
        }
    }
    delete it;
    return QDocCollection::success;
}

int QDocCollection::find(QJsonObject query, QJsonArray* pReply) {
    QList<QByteArray> keys;
    int r = this->find(query, pReply, keys);
    if (r != QDocCollection::success) {
        return r;
    }
    return QDocCollection::success;
}

//---

int QDocCollection::printAll() {
    qDebug() << "printAll" << endl;
    QDocKVIterator* it = this->kvdb->newIterator();
    for (it->seekToFirst(); it->isValid(); it->next()) {
        QByteArray key = it->key();
        if (key.mid(0, 3) == "in:") {
            qDebug() << key << " : " << it->value() << endl;
        } else {
            qDebug() << key << " : " << this->getJsonValue(it) << endl;
        }
    }
    delete it;
    return 0;
}

int QDocCollection::observe(QJsonObject query) {
    int id = this->nextObserverId++;
    td_s_observer observer;
    observer.id = id;
    observer.query = query;
    observer.triggered = true;
    this->find(query, &observer.reply);
    this->observers[id] = observer;
    this->emitObserver(id);
    return id;
}

int QDocCollection::unobserve(int id) {
    if (this->observers.contains(id)) {
        this->observers.remove(id);
    }
    return QDocCollection::success;
}

QDocCollectionTransaction* QDocCollection::newTransaction() {
    return new QDocCollectionTransaction(this->kvdb->newTransaction(), this->baseDir, this->pIdGen, &this->observers);
}

QDocCollection* QDocCollection::open(QString collectionDir, QDocIdGen *pIdGen) {
    QDocCollection* pColl = new QDocCollection(collectionDir);
    if (!pColl->isOpened) {
        delete pColl;
        return NULL;
    }
    if (pColl != NULL) {
        pColl->pIdGen = pIdGen;
    }
    return pColl;
}

QString QDocCollection::getBaseDir() {
    return this->baseDir;
}

QDocCollection::QDocCollection(QString collectionDir) {
    isOpened = false;
    this->baseDir = collectionDir;
    this->kvdb = new QDocKVLevelDB("file://" + collectionDir);
    isOpened = this->kvdb->isValid();
    if (isOpened) {
        this->createIndex("", "__START_INDEXES");
        this->getIndexes();
    } else {
        this->lastError = this->kvdb->getLastError();
    }
    this->nextObserverId = 0;
}

QDocCollection::QDocCollection(QDocKVInterface *kvdb, QString collectionDir, QDocIdGen *pIdGen) {
    isOpened = false;
    this->baseDir = collectionDir;
    this->kvdb = kvdb;
    this->pIdGen = pIdGen;
    if (this->kvdb == NULL) {
        return;
    }
    isOpened = this->kvdb->isValid();
    if (isOpened) {
        this->getIndexes();
    }
    this->nextObserverId = 0;
}

QDocCollection::~QDocCollection() {
    if (this->kvdb != NULL) {
        delete this->kvdb;
    }
}

//---

int QDocCollectionTransaction::createIndex(QString fieldName, QString indexName) {
    QByteArray key = QString("in:" + fieldName).toLocal8Bit();
    QByteArray value;
    int r = this->kvdb->getValue(key, value);
    if (r == QDocKVInterface::success) return QDocCollection::success;

    value = indexName.toLocal8Bit();
    r = this->kvdb->putValue(key, value);
    if (r != QDocKVInterface::success) {
        this->lastError = this->kvdb->getLastError();
        return QDocCollection::errorDatabase;
    }
    if (fieldName == "") return QDocCollection::success;

    r = this->addIndexValue(indexName, QJsonValue(QJsonValue::Undefined), "__START_INDEX_VALUES");
    if (r != QDocCollection::success) {
        return r;
    }

    QDocKVIterator* it = this->kvdb->newIterator();

    for (it->seekToFirst(); it->isValid() && it->key().mid(0, 2) == "d:"; it->next()) {
        QString key = QString::fromLocal8Bit(it->key());
        QJsonValue value = this->getJsonValue(it);
        if (value.isObject()) {
            QJsonValue jsonValue = QDocUtils::getJsonValueByPath(value, fieldName);
            r = this->addJsonValueToIndex(indexName, jsonValue, key);
            if (r != QDocCollection::success) {
                delete it;
                return r;
            }
        }
    }
    delete it;
    this->indexes[fieldName] = indexName;
    return QDocCollection::success;
}

int QDocCollectionTransaction::set(QJsonObject& query, QJsonArray& docs) {

    QList<QByteArray> keys;
    QJsonArray reply;
    this->find(query, &reply, keys);

    for (QJsonArray::iterator it = docs.begin(); it != docs.end(); it++) {
        if (it->isObject()) {
            bool valid = false;
            int r = this->checkValidR(query, *it, "", valid);
            if (r != QDocCollection::success) {
                return r;
            }
            if (valid) {
                QString id;
                r = this->insert(it->toObject(), id, true);
                if (r != QDocCollection::success) {
                    return r;
                }
                QByteArray key = QString("d:" + id).toLocal8Bit();
                if (keys.contains(key)) {
                    keys.removeAll(key);
                }
            }
        }
    }
    for (QList<QByteArray>::iterator it = keys.begin(); it != keys.end(); it++) {
        int r = this->removeByKey(*it);
        if (r != QDocCollection::success) {
            return r;
        }
    }
    return QDocCollection::success;
}

int QDocCollectionTransaction::remove(QJsonObject& query) {
    QList<QByteArray> keys;
    QJsonArray reply;
    int r = this->find(query, &reply, keys);
    if (r != QDocCollection::success) {
        return r;
    }
    foreach (QByteArray key, keys) {
        r = this->removeByKey(key);
        if (r != QDocCollection::success) {
            return r;
        }
    }
    return QDocCollection::success;
}

int QDocCollectionTransaction::removeId(QString &docId) {
    QByteArray linkKey = QString("d:" + docId).toLocal8Bit();
    int r = this->removeByKey(linkKey);
    return r;
}

int QDocCollectionTransaction::writeTransaction() {
    if (this->kvdb->writeTransaction() != QDocKVInterface::success) {
        this->lastError = this->kvdb->getLastError();
        return QDocCollectionTransaction::errorWriteTransaction;
    }
    foreach(int observerId, this->observers.keys()) {
        if (this->observers[observerId].triggered) {
            (*this->baseObservers)[observerId].reply = this->observers[observerId].reply;
            (*this->baseObservers)[observerId].triggered = true;
        }
    }
    return QDocCollection::success;
}

QDocCollectionTransaction::QDocCollectionTransaction(QDocKVInterface *kvdb, QString collectionDir, QDocIdGen *pIdGen, QHash<int, td_s_observer>* observers)
    : QDocCollection(kvdb, collectionDir, pIdGen) {
    this->baseObservers = observers;
    this->observers = *observers;
}
