
#include <QDebug>
#include <QMap>
#include <QUrl>
#include <QUrlQuery>

#include "qdocdbconnector.h"

// database clients singleton
QMap<QString, QDocdbClient*> dbClients;

void QDocdbConnector::setLastError(QString lastError) {
    if (this->lastError != lastError) {
        this->lastError = lastError;
        emit this->errChanged();
    }
}

void QDocdbConnector::setUrl(QString url) {
    if (this->_url == url) return;
    this->_url = url;

    QUrl u(url);
    if (u.scheme() != "qdocdb") {
        this->setLastError("Error: scheme must be 'qdocdb'");
        return;
    }
    QString serverName = u.host();
    QString database = u.path().mid(1);
    QString collection;
    if (u.hasQuery()) {
        QUrlQuery urlQuery(u);
        collection = urlQuery.queryItemValue("collection");
    }
    this->unobserve();

    if ((serverName.isEmpty()) || (database.isEmpty()) || (collection.isEmpty())) {
        this->setLastError("Error: Invalid url: " + url);
        this->urlValid = false;
        emit this->validChanged();
        return;
    } else {
        this->urlValid = true;
    }

    if (dbClients.contains(serverName)) {
        this->dbClient = dbClients[serverName];
    } else {
        this->dbClient = new QDocdbClient();
        dbClient->connectToServer(serverName);
        if (!dbClient->isConnected()) {
            this->setLastError("Can't connect to database server " + serverName);
            delete this->dbClient;
            this->dbClient = NULL;
        } else {
            dbClients[serverName] = this->dbClient;
        }
    }
    if ((!this->_query.isEmpty()) || (this->_allowEmptyQuery)) {
        this->observe();
    }
    emit this->validChanged();
}

void QDocdbConnector::setQuery(QJsonObject query) {
    if (this->_query != query) {
        this->unobserve();
        this->_query = query;
        this->observe();
    }
}

void QDocdbConnector::setAllowEmptyQuery(bool allowEmptyQuery) {
    if (this->_allowEmptyQuery != allowEmptyQuery) {
        this->unobserve();
        this->_allowEmptyQuery = allowEmptyQuery;
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

QVariantList QDocdbConnector::find(QJsonObject query, QString snapshot) {
    if (!this->valid()) return QVariantList();
    QVariantList replyList;
    int r = dbClient->find(this->_url, query.toVariantMap(), replyList, QVariantMap(), snapshot);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
    }
    return replyList;
}

QVariantMap QDocdbConnector::findOne(QJsonObject query, QString snapshot) {
    if (!this->valid()) return QVariantMap();
    QVariantMap reply;
    int r = dbClient->findOne(this->_url, query.toVariantMap(), reply, QVariantMap(), snapshot);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
    }
    return reply;
}

int QDocdbConnector::count(QJsonObject query, QString snapshot) {
    if (!this->valid()) return 0;
    int reply;
    int r = dbClient->count(this->_url, query.toVariantMap(), reply, QVariantMap(), snapshot);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
    }
    return reply;
}

QDocdbConnector::resultEnum QDocdbConnector::insert(QJsonObject doc, int transactionId) {
    if (!this->valid()) return QDocdbConnector::error;
    QByteArray docId;
    int r = dbClient->insert(this->_url, doc.toVariantMap(), docId, transactionId);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::error;
    }
    return QDocdbConnector::success;
}

QDocdbConnector::resultEnum QDocdbConnector::remove(QJsonObject query, int transactionId) {
    if (!this->valid()) return QDocdbConnector::error;
    int r = dbClient->remove(this->_url, query.toVariantMap(), transactionId);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::error;
    }
    return QDocdbConnector::success;
}

QDocdbConnector::resultEnum QDocdbConnector::removeId(QString docId, int transactionId) {
    if (!this->valid()) return QDocdbConnector::error;
    int r = dbClient->removeId(this->_url, docId.toLocal8Bit(), transactionId);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::error;
    }
    return QDocdbConnector::success;
}

int QDocdbConnector::newTransaction() {
    if (!this->valid()) return QDocdbConnector::error;
    return dbClient->newTransaction(this->_url);
}

QDocdbConnector::resultEnum QDocdbConnector::writeTransaction(int transactionId) {
    if (!this->valid()) return QDocdbConnector::error;
    int r = dbClient->writeTransaction(transactionId);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::error;
    }
    return QDocdbConnector::success;
}

QDocdbConnector::resultEnum QDocdbConnector::discardTransaction(int transactionId) {
    if (!this->valid()) return QDocdbConnector::error;
    int r = dbClient->discardTransaction(transactionId);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::error;
    }
    return QDocdbConnector::success;
}

QDocdbConnector::resultEnum QDocdbConnector::removeQueryResults() {
    if (!this->valid()) return QDocdbConnector::error;
    int r = dbClient->set(this->_url, this->_query.toVariantMap(), QVariantList());
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::error;
    }
    return QDocdbConnector::success;
}

void QDocdbConnector::setValue(QJsonArray value) {
    if (!this->valid()) return;
    int r = dbClient->set(this->_url, this->_query.toVariantMap(), value.toVariantList());
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
    }
}

QJsonObject QDocdbConnector::valueOne() {
    if (!this->_value.count()) {
        return QJsonObject();
    }
    return this->_value[0].toObject();
}

void QDocdbConnector::setValueOne(QJsonObject value) {
    QJsonArray arrValue = this->_value;
    if (!arrValue.count()) {
        arrValue.append(value);
    } else {
        arrValue[0] = QJsonValue(value);
    }
    this->setValue(arrValue);
}

void QDocdbConnector::observe() {
    if (this->valid()) {
        this->observeId = dbClient->observe(this, this->_url, this->_query.toVariantMap(), this->_queryOptions.toVariantMap());
    }
}

void QDocdbConnector::unobserve() {
    if (this->valid()) {
        dbClient->unobserve(this->_url, this->observeId);
    }
}

void QDocdbConnector::observeQueryChanged(int, QJsonArray& reply) {
    if (this->_value != reply) {
        this->_value = reply;
        emit this->valueChanged();
        emit this->valueOneChanged();
    }
}

QDocdbConnector::resultEnum QDocdbConnector::createIndex(QString field, QString indexName) {
    if (!this->valid()) return QDocdbConnector::error;
    int r = dbClient->createIndex(this->_url, field, indexName);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::error;
    }
    return QDocdbConnector::success;
}

QDocdbConnector::resultEnum QDocdbConnector::newSnapshot(QString snapshotName) {
    if (!this->valid()) return QDocdbConnector::error;
    int r = dbClient->newSnapshot(this->_url, snapshotName);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::error;
    }
    return QDocdbConnector::success;
}

QDocdbConnector::resultEnum QDocdbConnector::revertToSnapshot(QString snapshotName) {
    if (!this->valid()) return QDocdbConnector::error;
    int r = dbClient->revertToSnapshot(this->_url, snapshotName);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::error;
    }
    return QDocdbConnector::success;
}

QDocdbConnector::resultEnum QDocdbConnector::removeSnapshot(QString snapshotName) {
    if (!this->valid()) return QDocdbConnector::error;
    int r = dbClient->removeSnapshot(this->_url, snapshotName);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::error;
    }
    return QDocdbConnector::success;
}

bool QDocdbConnector::valid() {
    if (this->dbClient == NULL) return false;
    return (this->dbClient->isConnected()) && (this->urlValid);
}

QDocdbConnector::QDocdbConnector() {
    this->_value = QJsonArray();
    this->_allowEmptyQuery = false;
    this->_query = QJsonObject();
    this->observeId = -1;
    this->dbClient = NULL;
}

QDocdbConnector::~QDocdbConnector() {
    for (QMap<QString, QDocdbClient*>::iterator it = dbClients.begin(); it != dbClients.end();) {
        QDocdbClient* client = it.value();
        delete client;
        it = dbClients.erase(it);
    }
}
