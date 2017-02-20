
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

void QDocdbConnector::insert(QJsonObject doc) {
    if ((this->pDatabase != NULL) && (this->_collection != "")) {
        this->_value.append(QJsonValue(doc));
        this->pDatabase->collection(this->_collection)->set(this->_query, this->_value);
    }
}

void QDocdbConnector::remove(QJsonObject query) {
    if ((this->pDatabase != NULL) && (this->_collection != "")) {
        this->pDatabase->collection(this->_collection)->remove(query);
    }
}

void QDocdbConnector::removeId(QString docId) {
    if ((this->pDatabase != NULL) && (this->_collection != "")) {
        this->pDatabase->collection(this->_collection)->removeId(docId);
    }
}

void QDocdbConnector::removeQueryResults() {
    if ((this->pDatabase != NULL) && (this->_collection != "")) {
        this->_value = QJsonArray();
        this->pDatabase->collection(this->_collection)->set(this->_query, this->_value);
    }
}

void QDocdbConnector::setValue(QJsonArray value) {
    if ((this->pDatabase != NULL) && (this->_collection != "")) {
        this->pDatabase->collection(this->_collection)->set(this->_query, value);
    }
}

void QDocdbConnector::observe() {
    if ((this->pDatabase != NULL) && (this->_collection != "")) {
        this->pDatabase->observe(this->pDatabase->collection(this->_collection), this->_query, this);
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

QString QDocdbConnector::createIndex(QString fieldName, QString indexName) {
    if ((this->pDatabase != NULL) && (this->_collection != "")) {
        QDocCollection* pColl = this->pDatabase->collection(this->_collection);
        pColl->createIndex(fieldName, indexName);
        return pColl->getLastError();
    }
    return "ERROR: Connection not setup";
}

QDocdbConnector::QDocdbConnector() {
    this->_value = QJsonArray();
    this->_query = QJsonObject();
    this->pDatabase = NULL;
    qRegisterMetaType<QJsonArray>("QJsonArray&");
}

QDocdbConnector::~QDocdbConnector() {
}
