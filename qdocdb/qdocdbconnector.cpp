
#include <QDebug>
#include <QHash>
#include <QStandardPaths>

#include "qdocdatabase.h"
#include "qdoccollection.h"
#include "qdocdbconnector.h"

// Databases singleton
QHash<QString, QDocDatabase*> databaseHash;

void QDocdbConnector::setDatabase(QString database) {
    if (this->_database != database) {
        this->unobserve();
        delete this->pDatabase;
        databaseHash.remove(this->_database);
        if (databaseHash.contains(database)) {
            this->pDatabase = databaseHash[database];
        } else {
            this->pDatabase = new QDocDatabase();
            if (pDatabase->open(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/" + database) != QDocDatabase::success) {
                delete pDatabase;
            } else {
                databaseHash[database] = pDatabase;
            }
        }
        this->_database = database;
        this->observe();
    }
}

void QDocdbConnector::setCollection(QString collection) {
    if (this->_collection != collection) {
        this->unobserve();
        this->_collection = collection;
        this->observe();
    }
}

void QDocdbConnector::setQuery(QJsonObject query) {
    if (this->_query != query) {
        this->unobserve();
        this->_query = query;
        this->observe();
    }
}

void QDocdbConnector::setQueryOptions(QJsonObject queryOptions) {
    if (this->_queryOptions != queryOptions) {
        this->unobserve();
        this->_queryOptions = queryOptions;
        this->observe();
    }
}

QDocdbConnector::resultEnum QDocdbConnector::find(QJsonObject query, QJsonArray& reply, QString snapshot) {
    if (!this->isValid()) return QDocdbConnector::error;
    QDocCollection* pColl;
    if (snapshot == "__CURRENT") {
        pColl = this->pDatabase->collection(this->_collection);
    } else {
        pColl = this->pDatabase->collection(this->_collection)->getSnapshot(snapshot);
    }
    int r = pColl->find(query, &reply);
    if (pColl->getClassType() == QDocCollection::typeReadOnlySnapshot) delete pColl;
    if (r != QDocCollection::success) {
        return QDocdbConnector::error;
    }
    return QDocdbConnector::success;
}

QDocdbConnector::resultEnum QDocdbConnector::insert(QJsonObject doc) {
    if (!this->isValid()) return QDocdbConnector::error;

    this->_value.append(QJsonValue(doc));
    QDocCollectionTransaction* tx = this->pDatabase->collection(this->_collection)->newTransaction();
    int r = tx->set(this->_query, this->_value);
    if (r == QDocCollection::success) {
        r = tx->writeTransaction();
    }
    if (r != QDocCollection::success) {
        this->lastError = tx->getLastError();
        delete tx;
        return QDocdbConnector::error;
    }
    delete tx;
    return QDocdbConnector::success;
}

QDocdbConnector::resultEnum QDocdbConnector::remove(QJsonObject query) {
    if (!this->isValid()) return QDocdbConnector::error;

    QDocCollectionTransaction* tx = this->pDatabase->collection(this->_collection)->newTransaction();
    int r = tx->remove(query);
    if (r == QDocCollection::success) {
        r = tx->writeTransaction();
    }
    if (r != QDocCollection::success) {
        this->lastError = tx->getLastError();
        delete tx;
        return QDocdbConnector::error;
    }
    delete tx;
    return QDocdbConnector::success;
}

QDocdbConnector::resultEnum QDocdbConnector::removeId(QString docId) {
    if (!this->isValid()) return QDocdbConnector::error;

    QDocCollectionTransaction* tx = this->pDatabase->collection(this->_collection)->newTransaction();
    int r = tx->removeById(docId.toLocal8Bit());
    if (r == QDocCollection::success) {
        r = tx->writeTransaction();
    }
    if (r != QDocCollection::success) {
        this->lastError = tx->getLastError();
        delete tx;
        return QDocdbConnector::error;
    }
    delete tx;
    return QDocdbConnector::success;
}

QDocdbConnector::resultEnum QDocdbConnector::removeQueryResults() {
    if (!this->isValid()) return QDocdbConnector::error;

    this->_value = QJsonArray();
    QDocCollectionTransaction* tx = this->pDatabase->collection(this->_collection)->newTransaction();
    int r = tx->set(this->_query, this->_value);
    if (r == QDocCollection::success) {
        r = tx->writeTransaction();
    }
    if (r != QDocCollection::success) {
        this->lastError = tx->getLastError();
        delete tx;
        return QDocdbConnector::error;
    }
    delete tx;
    return QDocdbConnector::success;
}

void QDocdbConnector::setValue(QJsonArray value) {
    if (!this->isValid()) return;

    QDocCollectionTransaction* tx = this->pDatabase->collection(this->_collection)->newTransaction();
    int r = tx->set(this->_query, value);
    if (r == QDocCollection::success) {
        r = tx->writeTransaction();
    }
    if (r != QDocCollection::success) {
        this->lastError = tx->getLastError();
    }
    delete tx;
}

void QDocdbConnector::observe() {
    if (this->isValid()) {
        this->pDatabase->observe(this->pDatabase->collection(this->_collection), this->_query, this->_queryOptions, this);
    }
}

void QDocdbConnector::unobserve() {
    if (this->pDatabase != NULL) {
        this->pDatabase->unobserve(this->pDatabase->collection(this->_collection), this);
    }
}

void QDocdbConnector::observeQueryChanged(QJsonArray& reply) {
    this->_value = reply;
    emit this->valueChanged();
}

QDocdbConnector::resultEnum QDocdbConnector::createIndex(QString field, QString indexName) {
    if (!this->isValid()) return QDocdbConnector::error;

    QDocCollection* pColl = this->pDatabase->collection(this->_collection);
    QDocCollectionTransaction* tx = pColl->newTransaction();

    int r = tx->createIndex(field, indexName);
    if (r == QDocCollection::success) {
        r = tx->writeTransaction();
    }
    if (r != QDocCollection::success) {
        this->lastError = tx->getLastError();
        delete tx;
        return QDocdbConnector::error;
    }
    delete tx;
    return QDocdbConnector::success;
}

QDocdbConnector::resultEnum QDocdbConnector::newSnapshot(QString snapshotName) {
    if (!this->isValid()) return QDocdbConnector::error;

    QDocCollection* pColl = this->pDatabase->collection(this->_collection);
    int r = pColl->newSnapshot(snapshotName);
    if (r != QDocCollection::success) {
        this->lastError = pColl->getLastError();
        return QDocdbConnector::error;
    }
    return QDocdbConnector::success;
}

QDocdbConnector::resultEnum QDocdbConnector::revertToSnapshot(QString snapshotName) {
    if (!this->isValid()) return QDocdbConnector::error;

    QDocCollection* pColl = this->pDatabase->collection(this->_collection);
    int r = pColl->revertToSnapshot(snapshotName);
    if (r != QDocCollection::success) {
        this->lastError = pColl->getLastError();
        return QDocdbConnector::error;
    }
    return QDocdbConnector::success;
}

QDocdbConnector::resultEnum QDocdbConnector::removeSnapshot(QString snapshotName) {
    if (!this->isValid()) return QDocdbConnector::error;
    QDocCollection* pColl = this->pDatabase->collection(this->_collection);
    int r = pColl->removeSnapshot(snapshotName);
    if (r != QDocCollection::success) {
        this->lastError = pColl->getLastError();
        return QDocdbConnector::error;
    }
    return QDocdbConnector::success;
}

bool QDocdbConnector::isValid() {
    return ((this->pDatabase != NULL) && (this->_collection != ""));
}

QDocdbConnector::QDocdbConnector() {
    this->_value = QJsonArray();
    this->_query = QJsonObject();
    this->pDatabase = NULL;
    qRegisterMetaType<QJsonArray>("QJsonArray&");
}

QDocdbConnector::~QDocdbConnector() {
}
