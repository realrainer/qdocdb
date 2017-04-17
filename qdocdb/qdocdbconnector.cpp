
#include <QDebug>
#include <QMap>
#include <QUrl>
#include <QUrlQuery>

#include "qdocdbconnector.h"

// database clients singleton
QMap<QString, QDocdbClient*> dbClients;
QMap<QDocdbClient*, int> dbClientUses;

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
    if (this->dbClient != NULL) {
        dbClientUses[this->dbClient]--;
    }

    if (dbClients.contains(serverName)) {
        this->dbClient = dbClients[serverName];
        dbClientUses[this->dbClient]++;
    } else {
        this->dbClient = new QDocdbClient();
        dbClient->connectToServer(serverName);
        if (!dbClient->isConnected()) {
            this->setLastError("Can't connect to database server " + serverName);
            delete this->dbClient;
            this->dbClient = NULL;
        } else {
            dbClients[serverName] = this->dbClient;
            dbClientUses[this->dbClient]++;
        }
    }
    if (this->dbClient != NULL) {
        connect(this, SIGNAL(destroyed(QObject*)), this->dbClient, SLOT(subscriberDestroyed(QObject*)), static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::UniqueConnection));
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

void QDocdbConnector::setQueryOptions(QJsonObject queryOptions) {
    if (this->_queryOptions != queryOptions) {
        this->unobserve();
        this->_queryOptions = queryOptions;
        this->observe();
    }
}

void QDocdbConnector::setObservable(bool observable) {
    if (this->_observable != observable) {
        this->_observable = observable;
        if (this->_observable) {
            this->observe();
        } else {
            this->unobserve();
        }
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

QString QDocdbConnector::insert(QJsonObject doc, int transactionId) {
    if (!this->valid()) return "";
    QByteArray docId;
    int r = dbClient->insert(this->_url, doc.toVariantMap(), docId, false, transactionId);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return "";
    }
    return QString::fromLatin1(docId);
}

QDocdbConnector::resultEnum QDocdbConnector::remove(QJsonObject query, int transactionId) {
    if (!this->valid()) return QDocdbConnector::Error;
    int r = dbClient->remove(this->_url, query.toVariantMap(), transactionId);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::Error;
    }
    return QDocdbConnector::Success;
}

QDocdbConnector::resultEnum QDocdbConnector::removeId(QString docId, int transactionId) {
    if (!this->valid()) return QDocdbConnector::Error;
    int r = dbClient->removeId(this->_url, docId.toLocal8Bit(), false, transactionId);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::Error;
    }
    return QDocdbConnector::Success;
}

int QDocdbConnector::newTransaction() {
    if (!this->valid()) return QDocdbConnector::Error;
    return dbClient->newTransaction(this->_url);
}

QDocdbConnector::resultEnum QDocdbConnector::writeTransaction(int transactionId) {
    if (!this->valid()) return QDocdbConnector::Error;
    int r = dbClient->writeTransaction(transactionId);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::Error;
    }
    return QDocdbConnector::Success;
}

QDocdbConnector::resultEnum QDocdbConnector::discardTransaction(int transactionId) {
    if (!this->valid()) return QDocdbConnector::Error;
    int r = dbClient->discardTransaction(transactionId);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::Error;
    }
    return QDocdbConnector::Success;
}

QDocdbConnector::resultEnum QDocdbConnector::removeQueryResults() {
    if (!this->valid()) return QDocdbConnector::Error;
    int r = dbClient->set(this->_url, this->_query.toVariantMap(), QVariantList());
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::Error;
    }
    return QDocdbConnector::Success;
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
    if ((this->valid()) && (this->_observable)) {
        this->observeId = dbClient->observe(this, this->_url, this->_query.toVariantMap(), this->_queryOptions.toVariantMap());
    }
}

void QDocdbConnector::unobserve() {
    if (this->valid()) {
        dbClient->unobserve(this->observeId);
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
    if (!this->valid()) return QDocdbConnector::Error;
    int r = dbClient->createIndex(this->_url, field, indexName);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::Error;
    }
    return QDocdbConnector::Success;
}

QDocdbConnector::resultEnum QDocdbConnector::newSnapshot(QString snapshotName) {
    if (!this->valid()) return QDocdbConnector::Error;
    int r = dbClient->newSnapshot(this->_url, snapshotName);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::Error;
    }
    return QDocdbConnector::Success;
}

QDocdbConnector::resultEnum QDocdbConnector::revertToSnapshot(QString snapshotName) {
    if (!this->valid()) return QDocdbConnector::Error;
    int r = dbClient->revertToSnapshot(this->_url, snapshotName);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::Error;
    }
    return QDocdbConnector::Success;
}

QDocdbConnector::resultEnum QDocdbConnector::removeSnapshot(QString snapshotName) {
    if (!this->valid()) return QDocdbConnector::Error;
    int r = dbClient->removeSnapshot(this->_url, snapshotName);
    if (r != QDocdbClient::success) {
        this->setLastError(dbClient->getLastError());
        return QDocdbConnector::Error;
    }
    return QDocdbConnector::Success;
}

bool QDocdbConnector::valid() {
    if ((!this->initComplete) || (this->dbClient == NULL)) return false;
    return (this->dbClient->isConnected()) && (this->urlValid);
}

void QDocdbConnector::componentComplete() {
    QQuickItem::componentComplete();
    this->initComplete = true;
    this->observe();
}

QDocdbConnector::QDocdbConnector(QString url, QJsonObject query, QJsonObject queryOptions) : QDocdbConnector()  {
    this->_query = query;
    this->_queryOptions = queryOptions;
    this->initComplete = true;
    this->setUrl(url);
}

QDocdbConnector::QDocdbConnector() {
    this->_value = QJsonArray();
    this->_query = QJsonObject();
    this->observeId = -1;
    this->dbClient = NULL;
    this->_observable = true;
    this->initComplete = false;
}

QDocdbConnector::~QDocdbConnector() {
    if (this->dbClient != NULL) {
        dbClientUses[this->dbClient]--;
        if (dbClientUses[this->dbClient] <= 0) {
            for (QMap<QString, QDocdbClient*>::iterator it = dbClients.begin(); it != dbClients.end();) {
                QDocdbClient* client = it.value();
                if (client == this->dbClient) {
                    it = dbClients.erase(it);
                } else {
                    it++;
                }
            }
            dbClientUses.remove(this->dbClient);
            delete this->dbClient;
        }
    }
}
