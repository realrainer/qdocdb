
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>
#include <QDataStream>
#include <QList>

#include "qdocutils.h"
#include "qdockvleveldb.h"
#include "qdockvmap.h"
#include "qdoccollection.h"

QDocCollection::iterator::iterator(QDocKVIterator* kvit, unsigned char snapshotId) {
    this->kvit = kvit;
    this->snapshotId = snapshotId;
}

QDocCollection::iterator::~iterator() {
    delete this->kvit;
}

QByteArray QDocCollection::iterator::key() {
    return this->prefix.mid(2);
}

QJsonValue QDocCollection::iterator::value() {
    unsigned char snapshotId;
    bool isSingle;
    return this->value(snapshotId, isSingle);
}

QJsonValue QDocCollection::iterator::value(unsigned char& snapshotId, bool& isSingle) {
    QByteArray valueData;

    bool find = false;
    isSingle = true;
    for (this->kvit->seek(this->beginKey); (this->kvit->isValid()) && (this->kvit->key().mid(0, this->prefix.size()) == this->prefix) && (!find); ) {
        QByteArray key = this->kvit->key();
        if (key.size() > (this->prefix.size() + 1)) {
            snapshotId = key[key.size() - 1];
            if (snapshotId <= this->snapshotId) {
                valueData = this->kvit->value();
                find = snapshotId == this->snapshotId;
            } else {
                isSingle = false;
            }
        }
        this->kvit->next();
    }
    if (!valueData.isEmpty()) {
        return QDocSerialize::unmarshal(valueData);
    } else {
        return QJsonValue(QJsonValue::Undefined);
    }
}

void QDocCollection::iterator::setupPrefix() {
    if (this->kvit->isValid()) {
        this->beginKey = this->kvit->key();
        if (this->beginKey.mid(0, 2) == "d:") {
            this->prefix = this->beginKey.mid(0, this->beginKey.size() - 1);
        } else {
            this->prefix = "";
        }
    }
}

void QDocCollection::iterator::seekToFirst() {
    this->kvit->seekToFirst();
    this->setupPrefix();
}

void QDocCollection::iterator::next() {
    for (; this->kvit->isValid() && this->kvit->key().mid(0, this->prefix.size()) == this->prefix; this->kvit->next());
    this->setupPrefix();
}

void QDocCollection::iterator::seek(QByteArray id) {
    QByteArray linkKey = "d:";
    linkKey.append(id);
    linkKey.append(":");
    this->kvit->seek(linkKey);
    this->setupPrefix();
}

bool QDocCollection::iterator::isValid() {
    return ((this->kvit->isValid()) && (!this->prefix.isEmpty()));
}

//---

QDocCollection::iterator* QDocCollection::newIterator() {
    QDocKVIterator* it = this->kvdb->newIterator();
    it->seekToFirst();
    return new QDocCollection::iterator(it, this->snapshotId);
}

//---

QString QDocCollection::getLastError() {
    return this->lastError;
}

//---

QDocCollection::classTypeEnum QDocCollection::getClassType() {
    return this->classType;
}

QHash<QString, QString> QDocCollection::getIndexes() {
    QByteArray key = "in:";
    QByteArray indexesData;
    this->kvdb->getValue(key, indexesData);
    return QDocSerialize::unmarshalIndexes(indexesData);
}

QByteArray QDocCollection::constructIndexValueStartKey(QString indexName, unsigned char snapshotId) {
    QByteArray key = QString("iv:" + indexName + ":").toLocal8Bit();
    key.append(snapshotId);
    key.append(":");
    return key;
}

QByteArray QDocCollection::constructIndexValueKey(QString indexName, QByteArray valueData) {
    QByteArray key = QDocCollection::constructIndexValueStartKey(indexName, this->snapshotId);
    key.append(valueData);
    return key;
}

QByteArray QDocCollection::constructIndexValueKey(QString indexName, QJsonValue jsonValue) {
    return this->constructIndexValueKey(indexName, QDocSerialize::marshal(jsonValue));
}

QJsonValue QDocCollection::getJsonValue(QByteArray id) {
    QDocCollection::iterator* it = this->newIterator();
    it->seek(id);
    if (!it->isValid()) {
        delete it;
        return QJsonValue(QJsonValue::Undefined);
    }
    QJsonValue value = it->value();
    delete it;
    return value;
}

QJsonValue QDocCollection::getJsonValue(QByteArray id, unsigned char& snapshotId, bool& isSingle) {
    QDocCollection::iterator* it = this->newIterator();
    it->seek(id);
    if (!it->isValid()) {
        delete it;
        return QJsonValue(QJsonValue::Undefined);
    }
    QJsonValue value = it->value(snapshotId, isSingle);
    delete it;
    return value;
}

QJsonValue QDocCollection::getJsonValueByLinkKey(QByteArray linkKey) {
    QByteArray valueData;
    int r = this->kvdb->getValue(linkKey, valueData);
    if (r != QDocKVInterface::success) {
        return QJsonValue(QJsonValue::Undefined);
    }
    return QDocSerialize::unmarshal(valueData);
}

int QDocCollection::getIndexValueLinkKeys(QString indexName, QJsonValue jsonValue, QList<QByteArray>& linkKeys) {
    QByteArray ivkey = constructIndexValueKey(indexName, jsonValue);
    QByteArray linkKeysData;
    int r = this->kvdb->getValue(ivkey, linkKeysData);
    if (r == QDocKVInterface::success) {
        linkKeys = QDocSerialize::unmarshalIndexValue(linkKeysData);
        return QDocCollection::success;
    }
    return QDocCollection::errorDatabase;
}

int QDocCollection::getSnapshotsValue(QList<QString> &snapshots) {
    QByteArray valueData;
    int r = this->kvdb->getValue("s:", valueData);
    if (r != QDocKVInterface::success) {
        this->lastError = this->kvdb->getLastError();
        return QDocCollection::errorDatabase;
    }
    snapshots = QDocSerialize::unmarshalSnapshotsValue(valueData);
    return QDocCollection::success;
}

//---

int QDocCollection::getLastSnapshotId(unsigned char &snapshotId) {

    QList<QString> snapshots;
    int r = this->getSnapshotsValue(snapshots);
    if (r != QDocCollection::success) return r;

    if (snapshots.isEmpty()) {
        this->lastError = "Invalid snapshots structure";
        return QDocCollection::errorDatabase;
    }
    snapshotId = snapshots.size() - 1;
    return QDocCollection::success;
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
            } else
            if (key == "$not") {
                valid1 = true;
                r = this->checkValidR(queryValue, docPart, curPath, valid1);
                if (r != QDocCollection::success) {
                    return r;
                }
                valid1 = !valid1;
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

//---

int QDocCollection::find(QJsonObject query, QJsonArray* pReply, QList<QByteArray>& ids, QJsonObject options) {

    ids = QList<QByteArray>();
    *pReply = QJsonArray();

    // try find indexes
    bool firstFind = true;
    QList<QByteArray> linkKeys;

    QHash<QString, QString> indexes = this->getIndexes();

    foreach(QString key, query.keys()) {
        if ((indexes.contains(key)) && (!query[key].isArray()) && (!query[key].isObject())) {
            QList<QByteArray> indexLinkKeys;
            int r = this->getIndexValueLinkKeys(indexes[key], query[key], indexLinkKeys);
            if (r != QDocCollection::success) {
                return r;
            }
            if (firstFind) {
                firstFind = false;
                linkKeys = indexLinkKeys;
            } else {
                linkKeys = QDocUtils::multiply(linkKeys, indexLinkKeys);
            }
        }
    }
    if (!firstFind) {
        for (QList<QByteArray>::iterator it = linkKeys.begin(); it != linkKeys.end(); it++) {
            QJsonValue jsonValue = this->getJsonValueByLinkKey(*it);
            if (jsonValue.isObject()) {
                bool valid = true;
                int r = this->checkValidR(query, jsonValue, "", valid);
                if (r != QDocCollection::success) {
                    return r;
                }
                if (valid) {
                    pReply->append(jsonValue);
                    ids.append(*it);
                }
            }
        }
        return QDocCollection::success;
    }

    // full search
    QDocCollection::iterator* it = this->newIterator();
    for (it->seekToFirst(); it->isValid(); it->next()) {
        QJsonValue jsonValue = it->value();
        if (jsonValue.isObject()) {
            bool valid = true;
            int r = this->checkValidR(query, jsonValue, "", valid);
            if (r != QDocCollection::success) {
                return r;
            }
            if (valid) {
                pReply->append(jsonValue);
                ids.append(it->key());
            }
        }
    }
    delete it;

    this->applyOptions(pReply, &ids, options);

    return QDocCollection::success;
}

void QDocCollection::applyOptions(QJsonArray *pReply, QList<QByteArray> *pIds, QJsonObject options) {

    if (options.contains("$orderBy")) {
        QString orderPath;
        bool orderUp = true;
        QJsonObject orderBy = options["$orderBy"].toObject();
        if (!orderBy.isEmpty()) {
            QJsonObject::iterator it = orderBy.begin();
            if (it.value().isDouble()) {
                orderPath = it.key();
                if (it.value().toInt() == 0) {
                    orderUp = false;
                }
            }
        }
        if (!orderPath.isEmpty()) {
            QString oper;
            if (orderUp) oper = "$gte"; else oper = "$lte";
            QJsonArray::iterator it = pReply->begin();
            QList<QByteArray>::iterator itl;
            if (pIds != NULL) itl = pIds->begin();
            while (it != pReply->end()) {
                QJsonArray::iterator it0 = it + 1;
                QList<QByteArray>::iterator itl0;
                if (pIds != NULL) itl0 = itl + 1;
                QJsonValue value = QDocUtils::getJsonValueByPath(*it, orderPath);
                while (it0 != pReply->end()) {
                    QJsonValue value0 = QDocUtils::getJsonValueByPath(*it0, orderPath);
                    if (QDocUtils::compare(value, value0, oper)) {
                        QJsonValue v = *it;
                        *it = *it0;
                        *it0 = v;
                        value = value0;
                        if (pIds != NULL) {
                            QByteArray bv = *itl;
                            *itl = *itl0;
                            *itl0 = bv;
                        }
                    }
                    it0++;
                    if (pIds != NULL) itl0++;
                }
                it++;
                if (pIds != NULL) itl++;
            }
        }
    }

    if (options.contains("$limit")) {
        int limit = options["$limit"].toInt();
        QJsonArray::iterator it = pReply->begin();
        QList<QByteArray>::iterator itl;
        if (pIds != NULL) itl = pIds->begin();
        int index = 0;
        while ((it != pReply->end()) && (index < limit)) {
            index++;
            it++;
            if (pIds != NULL) itl++;
        }
        while (it != pReply->end()) {
            it = pReply->erase(it);
            if (pIds != NULL) itl = pIds->erase(itl);
        }
    }
}

int QDocCollection::find(QJsonObject query, QJsonArray* pReply, QJsonObject options) {
    QList<QByteArray> ids;
    int r = this->find(query, pReply, ids, options);
    if (r != QDocCollection::success) {
        return r;
    }
    return QDocCollection::success;
}

int QDocCollection::count(QJsonObject query, int &replyCount, QJsonObject options) {
    QList<QByteArray> ids;
    QJsonArray reply;
    int r = this->find(query, &reply, ids, options);
    if (r != QDocCollection::success) {
        return r;
    }
    replyCount = ids.size();
    return QDocCollection::success;
}

//---

int QDocCollection::printAll() {
    qDebug() << "printAll" << endl;
    QDocKVIterator* it = this->kvdb->newIterator();
    for (it->seekToFirst(); it->isValid(); it->next()) {
        QByteArray linkKey = it->key();
        if (linkKey.mid(0, 3) == "in:") {
            QByteArray indexesValue = it->value();
            qDebug() << linkKey << " : " << QDocSerialize::unmarshalIndexes(indexesValue) << endl;
        } else
        if (linkKey == "s:") {
            QByteArray snapshotsValue = it->value();
            qDebug() << linkKey << " : " << QDocSerialize::unmarshalSnapshotsValue(snapshotsValue) << endl;
        } else
        if (linkKey.mid(0, 3) == "iv:") {
            QByteArray indexValue = it->value();
            qDebug() << linkKey << " : " << QDocSerialize::unmarshalIndexValue(indexValue) << endl;
        } else {
            QByteArray docValue = it->value();
            qDebug() << linkKey << " : " << QDocSerialize::unmarshal(docValue) << endl;
        }
    }
    delete it;
    return 0;
}

int QDocCollection::observe(QJsonObject query, QJsonObject queryOptions) {
    int id = this->nextObserverId++;
    td_s_observer observer;
    observer.id = id;
    observer.query = query;
    observer.queryOptions = queryOptions;
    observer.triggered = true;
    this->find(query, &observer.reply, queryOptions);
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

void QDocCollection::reloadObservers() {
    foreach (int id, this->observers.keys()) {
        this->find(this->observers[id].query, &this->observers[id].reply);
        this->observers[id].triggered = true;
    }
    this->emitObservers();
}

QDocCollectionTransaction* QDocCollection::newTransaction() {
    if (classType == QDocCollection::typeCollection) {
        QDocCollectionTransaction* tx = new QDocCollectionTransaction(this);
        return tx;
    } else {
        return NULL;
    }
}

int QDocCollection::newSnapshot(QString snapshotName) {
    if (classType != QDocCollection::typeCollection) {
        return QDocCollection::errorType;
    }

    unsigned char snapshotId;
    if (this->getSnapshotId(snapshotName, snapshotId) == QDocCollection::success) {
        this->lastError = "Snapshot is already exists";
        return QDocCollection::errorSnapshotIsExists;
    }

    QDocCollectionTransaction* tx = this->newTransaction();
    QList<QString> snapshots;
    int r = this->getSnapshotsValue(snapshots);
    if (r == QDocCollection::success) {
        snapshots[snapshots.size() - 1] = snapshotName;
        snapshots.append("__CURRENT");
        r = tx->putSnapshotsValue(snapshots);
        if (r == QDocCollection::success) {
            r = tx->copyIndexValues(this->snapshotId + 1, this->snapshotId);
            if (r == QDocCollection::success) {
                r = tx->writeTransaction();
            }
        }
    }
    if (r != QDocCollection::success) {
        return r;
    }
    this->snapshotId++;
    return QDocCollection::success;
}

int QDocCollection::getSnapshotId(QString snapshotName, unsigned char& snapshotId) {
    QList<QString> snapshots;
    int r = this->getSnapshotsValue(snapshots);
    if (r != QDocCollection::success) {
        return r;
    }
    int index = snapshots.indexOf(snapshotName);
    if (index >= 0) {
        snapshotId = (unsigned char)index;
        return QDocCollection::success;
    }
    return QDocCollection::errorInvalidObject;
}

QDocCollectionSnapshot* QDocCollection::getSnapshot(QString snapshotName) {
    if (classType != QDocCollection::typeCollection) return NULL;
    unsigned char snapshotId;
    this->getSnapshotId(snapshotName, snapshotId);
    return new QDocCollectionSnapshot(this, snapshotId);
}

int QDocCollection::revertToSnapshot(QString snapshotName) {
    unsigned char snapshotId;
    int r = this->getSnapshotId(snapshotName, snapshotId);
    if (r != QDocCollection::success) {
        return r;
    }
    QList<QString> snapshots;
    r = this->getSnapshotsValue(snapshots);
    if (r != QDocCollection::success) {
        return r;
    }
    QList<unsigned char> sIds;
    while (snapshots.size() > (snapshotId + 1)) {
        sIds.append(snapshots.size() - 1);
        snapshots.removeLast();
    }
    snapshots[snapshots.size() - 1] = "__CURRENT";

    QDocCollectionTransaction* tx = this->newTransaction();
    tx->putSnapshotsValue(snapshots);

    QDocKVIterator* it = this->kvdb->newIterator();
    QByteArray startKey;
    bool isSingle = false;
    for (it->seekToFirst(); it->isValid() && it->key().mid(0, 2) == "d:"; it->next()) {
        QByteArray key = it->key();
        if (key[key.size() - 2] == ':') {
            unsigned char sId = key[key.size() - 1];
            if (sIds.contains(sId)) {
                tx->kvdb->deleteValue(key);
            } else {
                isSingle = false;
            }
        } else {
            if ((isSingle) && (!startKey.isEmpty())) {
                tx->kvdb->deleteValue(startKey);
            }
            startKey = key;
            isSingle = true;
        }
    }
    if ((isSingle) && (!startKey.isEmpty())) {
        tx->kvdb->deleteValue(startKey);
    }

    QHash<QString, QString> indexes = this->getIndexes();
    foreach (QString fieldName, indexes.keys()) {
        QString indexName = indexes[fieldName];
        for (QList<unsigned char>::iterator sit = sIds.begin(); sit != sIds.end(); sit++) {
            QByteArray key = QDocCollection::constructIndexValueStartKey(indexName, *sit);
            for (it->seek(key); it->isValid() && it->key().mid(0, key.size()) == key; it->next()) {
                tx->kvdb->deleteValue(it->key());
            }
        }
    }

    delete it;

    r = tx->writeTransaction();
    if (r == QDocCollection::success) {
        this->snapshotId = snapshotId;
        this->reloadObservers();
    }
    delete tx;
    return r;
}

int QDocCollection::removeSnapshot(QString snapshotName) {
    unsigned char snapshotId;
    int r = this->getSnapshotId(snapshotName, snapshotId);
    if (r != QDocCollection::success) {
        return r;
    }

    QList<QString> snapshots;
    r = this->getSnapshotsValue(snapshots);
    if (r != QDocCollection::success) {
        return r;
    }

    QDocCollectionTransaction* tx = this->newTransaction();

    QDocKVIterator* it = this->kvdb->newIterator();
    QByteArray lastKey;
    QByteArray startKey;
    bool removeStartKey = false;
    for (it->seekToFirst(); it->isValid() && it->key().mid(0, 2) == "d:"; it->next()) {
        QByteArray key = it->key();
        if (key[key.size() - 2] == ':') {
            unsigned char sId = key[key.size() - 1];
            if (sId > snapshotId) {
                if (sId == snapshotId + 1) {
                    removeStartKey = true;
                }
                lastKey = key;
                key[key.size() - 1] = sId - 1;
                QByteArray valueData = it->value();
                QJsonValue jsonValue = QDocSerialize::unmarshal(valueData);
                if (!jsonValue.isUndefined()) {
                    tx->kvdb->putValue(key, valueData);
                    removeStartKey = false;
                } else {
                    tx->kvdb->deleteValue(key);
                }
            }
        } else {
            if (!lastKey.isEmpty()) {
                tx->kvdb->deleteValue(lastKey);
                lastKey.clear();
            }
            if ((!startKey.isEmpty()) && (removeStartKey)) {
                tx->kvdb->deleteValue(startKey);
            }
            startKey = key;
            removeStartKey = false;
        }
    }
    if (!lastKey.isEmpty()) {
        tx->kvdb->deleteValue(lastKey);
    }
    if ((!startKey.isEmpty()) && (removeStartKey)) {
        tx->kvdb->deleteValue(startKey);
    }

    QHash<QString, QString> indexes = this->getIndexes();
    foreach (QString fieldName, indexes.keys()) {
        QString indexName = indexes[fieldName];
        for (unsigned char sId = snapshotId; sId < snapshots.size(); sId++) {
            QByteArray startKey = QDocCollection::constructIndexValueStartKey(indexName, sId);
            for (it->seek(startKey); it->isValid() && it->key().mid(0, startKey.size()) == startKey; it->next()) {
                tx->kvdb->deleteValue(it->key());
            }
            QByteArray startKey1 = QDocCollection::constructIndexValueStartKey(indexName, sId + 1);
            for (it->seek(startKey1); it->isValid() && it->key().mid(0, startKey1.size()) == startKey1; it->next()) {
                QByteArray indexData = it->value();
                QList<QByteArray> indexValue = QDocSerialize::unmarshalIndexValue(indexData);
                bool changed = false;
                for (QList<QByteArray>::iterator iti = indexValue.begin(); iti != indexValue.end(); iti++) {
                    unsigned char dsId = (*iti)[(*iti).size() - 1];
                    if (dsId > snapshotId) {
                        (*iti)[(*iti).size() - 1] = dsId - 1;
                        changed = true;
                    }
                }
                if (changed) {
                    indexData = QDocSerialize::marshalIndexValue(indexValue);
                }
                QByteArray key = it->key();
                key.replace(startKey1, startKey);
                tx->kvdb->putValue(key, indexData);
            }

        }
    }
    delete it;

    snapshots.removeAt(snapshotId);
    r = tx->putSnapshotsValue(snapshots);
    if (r == QDocCollection::success) {
        r = tx->writeTransaction();
        if (r == QDocCollection::success) {
            this->snapshotId--;
            this->reloadObservers();
        }
    }
    delete tx;
    return r;
}

QDocCollection* QDocCollection::open(QString collectionDir, QDocIdGen *pIdGen, bool inMemory) {
    QDocCollection* pColl = new QDocCollection(collectionDir, inMemory);
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

QDocIdGen* QDocCollection::getIdGen() {
    return this->pIdGen;
}

QDocKVInterface* QDocCollection::getKVDB() {
    return this->kvdb;
}

QHash<int, td_s_observer>* QDocCollection::getObservers() {
    return &this->observers;
}

unsigned char QDocCollection::getSnapshotId() {
    return this->snapshotId;
}

QDocCollection::QDocCollection(QString collectionDir, bool inMemory) {
    isOpened = false;
    this->classType = QDocCollection::typeCollection;
    this->baseDir = collectionDir;
    this->nextObserverId = 0;
    if (inMemory) {
        this->kvdb = new QDocKVMap("map://" + collectionDir);
    } else {
        this->kvdb = new QDocKVLevelDB("file://" + collectionDir);
    }
    isOpened = this->kvdb->isValid();
    if (isOpened) {
        QDocCollectionTransaction* tx = this->newTransaction();
        int r = this->getLastSnapshotId(this->snapshotId);
        if (r != QDocCollection::success) {
            this->snapshotId = 0;
            QList<QString> snapshots;
            snapshots.append("__CURRENT");
            tx->putSnapshotsValue(snapshots);
        }
        tx->writeTransaction();
        delete tx;
    } else {
        this->lastError = this->kvdb->getLastError();
    }
    //...
    //this->printAll();
}

QDocCollection::QDocCollection(QDocKVInterface *kvdb, QString collectionDir, QDocIdGen *pIdGen, classTypeEnum classType) {
    isOpened = false;
    this->classType = classType;
    this->baseDir = collectionDir;
    this->kvdb = kvdb;
    this->pIdGen = pIdGen;
    this->nextObserverId = 0;
    if (this->kvdb == NULL) {
        return;
    }
    isOpened = this->kvdb->isValid();
}

QDocCollection::~QDocCollection() {
    if ((this->classType == QDocCollection::typeCollection) && (this->kvdb != NULL)) {
        delete this->kvdb;
    }
}

//---

int QDocCollectionTransaction::putIndexes(QHash<QString, QString> &indexes) {
    QByteArray indexesData = QDocSerialize::marshalIndexes(indexes);
    int r = this->kvdb->putValue("in:", indexesData);
    if (r != QDocKVInterface::success) {
        this->lastError = this->kvdb->getLastError();
        return QDocCollection::errorDatabase;
    }
    return QDocCollection::success;
}

QByteArray QDocCollectionTransaction::constructDocumentLinkKey(QByteArray id, unsigned char snapshotId) {
    QByteArray linkKey = QDocCollectionTransaction::constructDocumentStartKey(id);
    linkKey.append(snapshotId);
    return linkKey;
}

QByteArray QDocCollectionTransaction::constructDocumentLinkKey(QString id) {
    return this->constructDocumentLinkKey(id.toLocal8Bit());
}

QByteArray QDocCollectionTransaction::constructDocumentLinkKey(QByteArray id) {
    return QDocCollectionTransaction::constructDocumentLinkKey(id, this->snapshotId);
}

QByteArray QDocCollectionTransaction::constructDocumentStartKey(QByteArray id) {
    return QByteArray("d:") + id + ":";
}

int QDocCollectionTransaction::putJsonValue(QByteArray id, QJsonValue jsonValue) {
    int r = this->kvdb->putValue(this->constructDocumentStartKey(id), "");
    if (r == QDocKVInterface::success) {
        r = this->kvdb->putValue(this->constructDocumentLinkKey(id), QDocSerialize::marshal(jsonValue));
    }
    if (r != QDocKVInterface::success) {
        this->lastError = this->kvdb->getLastError();
        return QDocCollection::errorDatabase;
    }
    return QDocCollection::success;
}

int QDocCollectionTransaction::createIndex(QString fieldName, QString indexName) {

    QHash<QString, QString> indexes = this->getIndexes();
    if (indexes.contains(fieldName)) {
        return QDocCollection::success;
    }

    QByteArray ivkey = this->constructIndexValueKey(indexName, QByteArray());
    int r = this->kvdb->putValue(ivkey, "");
    if (r != QDocKVInterface::success) {
        this->lastError = this->kvdb->getLastError();
        return QDocCollection::errorDatabase;
    }

    QDocCollection::iterator* it = this->newIterator();

    for (it->seekToFirst(); it->isValid(); it->next()) {
        QJsonValue value = it->value();
        if (value.isObject()) {
            QJsonValue jsonValue = QDocUtils::getJsonValueByPath(value, fieldName);
            r = this->addIndexValue(indexName, jsonValue, it->key());
            if (r != QDocCollection::success) {
                delete it;
                return r;
            }
        }
    }

    indexes[fieldName] = indexName;
    r = this->putIndexes(indexes);
    if (r != QDocCollection::success) {
        return r;
    }
    return QDocCollection::success;
}

int QDocCollectionTransaction::addIndexValue(QString indexName, QJsonValue jsonValue, QByteArray id) {
    QByteArray ivkey = constructIndexValueKey(indexName, jsonValue);
    QByteArray linkKeysData;
    this->kvdb->getValue(ivkey, linkKeysData);

    QList<QByteArray> linkKeys = QDocSerialize::unmarshalIndexValue(linkKeysData);

    QByteArray linkKey = this->constructDocumentLinkKey(id);
    if (linkKeys.contains(linkKey)) {
        return QDocCollection::success;
    }
    linkKeys.append(linkKey);
    int r = this->kvdb->putValue(ivkey, QDocSerialize::marshalIndexValue(linkKeys));
    if (r != QDocKVInterface::success) {
        this->lastError = this->kvdb->getLastError();
        return QDocCollection::errorDatabase;
    }
    return QDocCollection::success;
}

int QDocCollectionTransaction::addIndexValue(QString indexName, QJsonValue jsonValue, QList<QByteArray> ids) {
    foreach(QByteArray id, ids) {
        int r = this->addIndexValue(indexName, jsonValue, id);
        if (r != QDocCollection::success) {
            return r;
        }
    }
    return QDocCollection::success;
}

int QDocCollectionTransaction::removeIndexValue(QString indexName, QJsonValue jsonValue, QByteArray linkKey) {
    QByteArray ivkey = constructIndexValueKey(indexName, jsonValue);
    QByteArray linkKeysData;
    int r = this->kvdb->getValue(ivkey, linkKeysData);
    if (r != QDocKVInterface::success) {
        this->lastError = this->kvdb->getLastError();
        return QDocCollection::errorDatabase;
    }
    QList<QByteArray> linkKeys = QDocSerialize::unmarshalIndexValue(linkKeysData);

    if (!linkKeys.contains(linkKey)) {
        return QDocCollection::success;
    }
    linkKeys.removeAll(linkKey);
    if (linkKeys.size()) {
        r = this->kvdb->putValue(ivkey, QDocSerialize::marshalIndexValue(linkKeys));
    } else {
        r = this->kvdb->deleteValue(ivkey);
    }
    if (r != QDocKVInterface::success) {
        this->lastError = this->kvdb->getLastError();
        return QDocCollection::errorDatabase;
    }
    return QDocCollection::success;
}

int QDocCollectionTransaction::putSnapshotsValue(QList<QString>& snapshots) {
    QByteArray valueData = QDocSerialize::marshalSnapshotsValue(snapshots);
    int r = this->kvdb->putValue("s:", valueData);
    if (r != QDocKVInterface::success) {
        this->lastError = this->kvdb->getLastError();
        return QDocCollection::errorDatabase;
    }
    return QDocCollection::success;
}

int QDocCollectionTransaction::copyIndexValues(unsigned char dstSnapshotId, unsigned char srcSnapshotId) {
    QHash<QString, QString> indexes = this->getIndexes();
    foreach(QString fieldName, indexes.keys()) {
        QString indexName = indexes[fieldName];
        QByteArray srcStartKey = QDocCollection::constructIndexValueStartKey(indexName, srcSnapshotId);
        QByteArray dstStartKey = QDocCollection::constructIndexValueStartKey(indexName, dstSnapshotId);
        QDocKVIterator* it = this->kvdb->newIterator();
        for (it->seek(srcStartKey); it->isValid() && it->key().mid(0, srcStartKey.size()) == srcStartKey; it->next()) {
            QByteArray dstIndexKey = dstStartKey + it->key().mid(srcStartKey.size());
            int r = this->kvdb->putValue(dstIndexKey, it->value());
            if (r != QDocKVInterface::success) {
                this->lastError = this->kvdb->getLastError();
                return QDocCollection::errorDatabase;
            }
        }
        delete it;
    }
    return QDocCollection::success;
}

int QDocCollectionTransaction::insert(QJsonObject doc, QByteArray& id, bool overwrite) {
    QJsonObject::iterator i_id = doc.find("_id");
    if (i_id == doc.end()) {
        id = this->pIdGen->getId();
        doc.insert("_id", QJsonValue(QString::fromLatin1(id)));
    } else {
        if (i_id.value().isString()) {
            id = i_id.value().toString().toLatin1();
        } else {
            this->lastError = "_id value is not string";
            return QDocCollection::errorInvalidObject;
        }
    }
    bool exists = false;
    if (this->getJsonValue(id) != QJsonValue::Undefined) {
        exists = true;
        if (!overwrite) {
            return QDocCollection::errorAlreadyExists;
        }
    }
    int r = this->putJsonValue(id, QJsonValue(doc));
    if (r != QDocCollection::success) {
        return r;
    }

    QHash<QString, QString> indexes = this->getIndexes();
    foreach (QString fieldName, indexes.keys()) {
        QJsonValue jsonValue = QDocUtils::getJsonValueByPath(QJsonValue(doc), fieldName);
        QString indexName = indexes[fieldName];
        r = this->addIndexValue(indexName, jsonValue, id);
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
            this->applyOptions(&this->observers[observeId].reply, NULL, this->observers[observeId].queryOptions);
            this->observers[observeId].triggered = true;
        }
    }
    return QDocCollection::success;
}

int QDocCollectionTransaction::set(QJsonObject& query, QJsonArray& docs) {

    QList<QByteArray> ids;
    QJsonArray reply;
    this->find(query, &reply, ids);

    for (QJsonArray::iterator it = docs.begin(); it != docs.end(); it++) {
        if (it->isObject()) {
            QByteArray id;
            bool find = false;
            for (QJsonArray::iterator itr = reply.begin(); (itr != reply.end()) && (!find); itr++) {
                if ((*itr) == (*it)) {
                    find = true;
                    id = itr->toObject()["_id"].toString().toLatin1();
                }
            }
            if (!find) {
                bool valid = false;
                int r = this->checkValidR(query, *it, "", valid);
                if (r != QDocCollection::success) {
                    return r;
                }
                if (valid) {
                    r = this->insert(it->toObject(), id, true);
                    if (r != QDocCollection::success) {
                        return r;
                    }
                }
            }
            if (ids.contains(id)) {
                ids.removeAll(id);
            }
        }
    }
    for (QList<QByteArray>::iterator it = ids.begin(); it != ids.end(); it++) {
        int r = this->removeById(*it);
        if (r != QDocCollection::success) {
            return r;
        }
    }
    return QDocCollection::success;
}

int QDocCollectionTransaction::remove(QJsonObject& query) {
    QList<QByteArray> ids;
    QJsonArray reply;
    int r = this->find(query, &reply, ids);
    if (r != QDocCollection::success) {
        return r;
    }
    foreach (QByteArray id, ids) {
        r = this->removeById(id);
        if (r != QDocCollection::success) {
            return r;
        }
    }
    return QDocCollection::success;
}


int QDocCollectionTransaction::removeById(QByteArray id) {
    unsigned char snapshotId;
    bool isSingle;
    QJsonValue jsonValue = this->getJsonValue(id, snapshotId, isSingle);
    if (!jsonValue.isObject()) {
        this->lastError = "value is not object";
        return QDocCollection::errorDatabase;
    }

    QByteArray linkKey = QDocCollectionTransaction::constructDocumentLinkKey(id, this->snapshotId);

    if (snapshotId == this->snapshotId) {
        int r = this->kvdb->deleteValue(linkKey);
        if ((r == QDocKVInterface::success) && (isSingle)) {
            r = this->kvdb->deleteValue(this->constructDocumentStartKey(id));
        }
        if (r != QDocKVInterface::success) {
            this->lastError = this->kvdb->getLastError();
            return QDocCollection::errorDatabase;
        }
    } else {
        int r = this->kvdb->putValue(linkKey, QDocSerialize::marshal(QJsonValue::Undefined));
        if (r != QDocKVInterface::success) {
            return r;
        }
    }

    linkKey = QDocCollectionTransaction::constructDocumentLinkKey(id, snapshotId);

    QJsonObject doc = jsonValue.toObject();
    QHash<QString, QString> indexes = this->getIndexes();
    foreach (QString fieldName, indexes.keys()) {
        QJsonValue jsonValue = QDocUtils::getJsonValueByPath(QJsonValue(doc), fieldName);
        QString indexName = indexes[fieldName];

        if (jsonValue.isArray()) {
            QJsonArray jsonArray = jsonValue.toArray();
            QJsonArray::iterator it;
            for (it = jsonArray.begin(); it != jsonArray.end(); it++) {
                int r = this->removeIndexValue(indexName, *it, linkKey);
                if (r != QDocCollection::success) {
                    this->lastError = this->kvdb->getLastError();
                    return QDocCollection::errorDatabase;
                }
            }
        } else {         
            int r = this->removeIndexValue(indexName, jsonValue, linkKey);
            if (r != QDocCollection::success) {
                this->lastError = this->kvdb->getLastError();
                return QDocCollection::errorDatabase;
            }
        }
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
                    this->applyOptions(&this->observers[observeId].reply, NULL, this->observers[observeId].queryOptions);
                    this->observers[observeId].triggered = true;
                }
                it++;
            }
        }
    }
    return QDocCollection::success;
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
    this->parentColl->emitObservers();
    return QDocCollection::success;
}

QDocCollectionTransaction::QDocCollectionTransaction(QDocCollection* pColl)
    : QDocCollection(pColl->getKVDB()->newTransaction(), pColl->getBaseDir(), pColl->getIdGen(), QDocCollection::typeTransaction) {
    this->baseObservers = pColl->getObservers();
    this->observers = *pColl->getObservers();
    this->snapshotId = pColl->getSnapshotId();
    this->parentColl = pColl;
}

//---
QDocCollectionSnapshot::QDocCollectionSnapshot(QDocCollection* pColl, unsigned char snapshotId)
    : QDocCollection(pColl->getKVDB(), pColl->getBaseDir(), pColl->getIdGen(), QDocCollection::typeReadOnlySnapshot) {
    this->snapshotId = snapshotId;
    this->parentColl = pColl;
}
