
#include <QDir>
#include <QUrl>
#include <QUrlQuery>
#include <QObject>
#include "qdocdbserver.h"

void QDocdbServer::newConnection() {
    while (this->tcpServer.hasPendingConnections()) {
        QTcpSocket* socket = this->tcpServer.nextPendingConnection();
        QDocdbLinkBase* client = new QDocdbLinkBase(socket);
        connect(client, SIGNAL(receive(QDocdbLinkObject*)), this, SLOT(receive(QDocdbLinkObject*)), Qt::DirectConnection);
        connect(client, SIGNAL(clientRemoved()), this, SLOT(clientRemoved()), Qt::DirectConnection);
        clients.append(client);
    }
}

void QDocdbServer::clientRemoved() {
    QDocdbLinkBase* client = (QDocdbLinkBase*)this->sender();
    for (QList<QDocdbLinkBase*>::iterator it = this->clients.begin(); it != this->clients.end();) {
        if ((*it) == client) {
            disconnect(client, SIGNAL(receive(QDocdbLinkObject*)), this, SLOT(receive(QDocdbLinkObject*)));
            disconnect(client, SIGNAL(clientRemoved()), this, SLOT(clientRemoved()));
            it = this->clients.erase(it);
        } else {
            it++;
        }
    }
    for (QMap<int, QDocdbLinkBase*>::iterator it = this->observeClient.begin(); it != this->observeClient.end();) {
        if (it.value() == client) {
            QDocCollection* coll = this->observeCollection[it.key()];
            coll->unobserve(it.key());
            it = this->observeClient.erase(it);
        } else {
            it++;
        }
    }
    for (QMap<int, QDocdbLinkBase*>::iterator it = this->transactionClient.begin(); it != this->transactionClient.end();) {
        if (it.value() == client) {
            int trId = it.key();
            QDocCollectionTransaction* tx = this->transactions[trId];
            delete tx;
            it = this->transactionClient.erase(it);
        } else {
            it++;
        }
    }
}

QDocDatabase* QDocdbServer::getDatabase(QString dbName) {
    if (this->databases.contains(dbName)) {
       return this->databases[dbName];
    } else {
        QDocDatabase* db = new QDocDatabase(this->commonConfig);
        if (db->open(this->baseDir + "/" + dbName) != QDocDatabase::success) {
            delete db;
            return NULL;
        } else {
            this->databases[dbName] = db;
            return db;
        }
    }
}

void QDocdbServer::receive(QDocdbLinkObject* linkObject) {
    QDocdbLinkBase* client = (QDocdbLinkBase*)this->sender();
    bool urlNeeded = ((linkObject->getType() != QDocdbLinkObject::typeWriteTransaction) &&
                      (linkObject->getType() != QDocdbLinkObject::typeDiscardTransaction));
    QString errorString;
    QDocDatabase* db;
    QString collName;
    QString dbName;
    bool inMemory = false;

    if (urlNeeded) {
        QUrl url(linkObject->get("url").toString());
        dbName = url.path().mid(1);
        if (url.hasQuery()) {
            QUrlQuery urlQuery(url);
            collName = urlQuery.queryItemValue("collection");
            if (urlQuery.hasQueryItem("persistent")) {
                if (urlQuery.queryItemValue("persistent") == "false") {
                    inMemory = true;
                }
            }
        }
        if (collName.isEmpty()) {
            errorString = "Collection name is empty";
        }
        db = this->getDatabase(dbName);
        if (db == NULL) {
            errorString = "Can't open database " + dbName;
        }
    }

    int id = linkObject->get("id").toInt();
    QDocdbLinkObject* replyObject = NULL;

    if (errorString.isEmpty()) {
        QString snapshotName = linkObject->get("snapshot").toString();
        if (snapshotName.isEmpty()) {
            snapshotName = "__CURRENT";
        }
        QDocCollection* coll;
        if (urlNeeded) {
            coll = db->collection(collName, inMemory);
            if (coll == NULL) {
                errorString = "Can't open collection " + collName + " in database " + dbName;
            }
        }
        if (errorString.isEmpty()) {
            int intType = linkObject->getType();
            switch (intType) {
            case QDocdbLinkObject::typeFind:
            case QDocdbLinkObject::typeFindOne:
            case QDocdbLinkObject::typeCount:
            case QDocdbLinkObject::typeGetModified: {
                QDocCollection* coll1;
                if (snapshotName != "__CURRENT") {
                    coll1 = coll->getSnapshot(snapshotName);
                } else {
                    coll1 = coll;
                }
                switch (intType) {
                case QDocdbLinkObject::typeFind: {
                    QJsonArray reply;
                    int r = coll1->find(QJsonObject::fromVariantMap(linkObject->get("query").toMap()), &reply, QJsonObject::fromVariantMap(linkObject->get("queryOptions").toMap()));
                    if (r != QDocCollection::success) {
                        errorString = coll->getLastError();
                    } else {
                        replyObject = QDocdbLinkObject::newLinkObject(id, QDocdbLinkObject::typeFindReply);
                        replyObject->set("reply", reply.toVariantList());
                    }
                    break;
                    }
                case QDocdbLinkObject::typeFindOne: {
                    QJsonObject reply;
                    int r = coll1->findOne(QJsonObject::fromVariantMap(linkObject->get("query").toMap()), &reply, QJsonObject::fromVariantMap(linkObject->get("queryOptions").toMap()));
                    if (r != QDocCollection::success) {
                        errorString = coll->getLastError();
                    } else {
                        replyObject = QDocdbLinkObject::newLinkObject(id, QDocdbLinkObject::typeFindOneReply);
                        replyObject->set("reply", reply.toVariantMap());
                    }
                    break;
                    }
                case QDocdbLinkObject::typeCount: {
                    int reply;
                    int r = coll1->count(QJsonObject::fromVariantMap(linkObject->get("query").toMap()), reply, QJsonObject::fromVariantMap(linkObject->get("queryOptions").toMap()));
                    if (r != QDocCollection::success) {
                        errorString = coll->getLastError();
                    } else {
                        replyObject = QDocdbLinkObject::newLinkObject(id, QDocdbLinkObject::typeCountReply);
                        replyObject->set("count", reply);
                    }
                    break;
                    }
                case QDocdbLinkObject::typeGetModified: {
                    QList<QByteArray> ids;
                    int r = coll1->getModified(ids);
                    if (r != QDocCollection::success) {
                        errorString = coll->getLastError();
                    } else {
                        replyObject = QDocdbLinkObject::newLinkObject(id, QDocdbLinkObject::typeGetModifiedReply);
                        QVariantList reply;
                        for (QList<QByteArray>::iterator it = ids.begin(); it != ids.end(); it++) {
                            reply.append(QVariant(*it));
                        }
                        replyObject->set("documentIds", reply);
                    }
                    break;
                    }
                }
                if (snapshotName != "__CURRENT") {
                    delete coll1;
                }
                break;
            }
            case QDocdbLinkObject::typeCreateIndex: {
                QDocCollectionTransaction* tx = coll->newTransaction();
                int r = tx->createIndex(linkObject->get("fieldName").toString(), linkObject->get("indexName").toString());
                if (r == QDocCollection::success) {
                    r = tx->writeTransaction();
                }
                if (r != QDocCollection::success) {
                    errorString = tx->getLastError();
                }
                delete tx;
                break;
            }
            case QDocdbLinkObject::typeNewTransaction: {
                QDocCollectionTransaction* tx = coll->newTransaction();
                int txId = this->getNextTransactionId();
                this->transactions[txId] = tx;
                replyObject = QDocdbLinkObject::newLinkObject(id, QDocdbLinkObject::typeNewTransactionReply);
                replyObject->set("transactionId", txId);
                break;
            }
            case QDocdbLinkObject::typeWriteTransaction: {
                int txId = linkObject->get("transactionId").toInt();
                if (!this->transactions.contains(txId)) {
                    errorString = "Invalid transaction number " + QString::number(txId);
                } else {
                    QDocCollectionTransaction* tx = this->transactions[txId];
                    int r = tx->writeTransaction();
                    if (r != QDocCollection::success) {
                        errorString = tx->getLastError();
                    }
                    delete tx;
                    this->transactions.remove(txId);
                }
                break;
            }
            case QDocdbLinkObject::typeDiscardTransaction: {
                int txId = linkObject->get("transactionId").toInt();
                if (!this->transactions.contains(txId)) {
                    errorString = "Invalid transaction number " + QString::number(txId);
                } else {
                    QDocCollectionTransaction* tx = this->transactions[txId];
                    delete tx;
                    this->transactions.remove(txId);
                }
                break;
            }
            case QDocdbLinkObject::typeInsert:
            case QDocdbLinkObject::typeRemove:
            case QDocdbLinkObject::typeRemoveId:
            case QDocdbLinkObject::typeSet: {
                int txId = linkObject->get("transactionId").toInt();
                QDocCollectionTransaction* tx = NULL;
                if (txId == -1) {
                     tx = coll->newTransaction();
                } else {
                    if (!this->transactions.contains(txId)) {
                        errorString = "Invalid transaction number " + QString::number(txId);
                    } else {
                        tx = this->transactions[txId];
                    }
                }
                if (tx != NULL) {
                    switch (intType) {
                    case QDocdbLinkObject::typeInsert: {
                        QByteArray docId;
                        int r = tx->insert(QJsonObject::fromVariantMap(linkObject->get("document").toMap()), docId, linkObject->get("overwrite").toBool());
                        if (r != QDocCollection::success) {
                            errorString = tx->getLastError();
                        } else {
                            replyObject = QDocdbLinkObject::newLinkObject(id, QDocdbLinkObject::typeInsertReply);
                            replyObject->set("documentId", docId);
                        }
                        break;
                    }
                    case QDocdbLinkObject::typeRemove: {
                        QJsonObject query = QJsonObject::fromVariantMap(linkObject->get("query").toMap());
                        int r = tx->remove(query);
                        if (r != QDocCollection::success) {
                            errorString = tx->getLastError();
                        }
                        break;
                    }
                    case QDocdbLinkObject::typeRemoveId: {
                        int r = tx->removeById(linkObject->get("documentId").toByteArray());
                        if (r != QDocCollection::success) {
                            errorString = tx->getLastError();
                        }
                        break;
                    }
                    case QDocdbLinkObject::typeSet: {
                        QJsonObject query = QJsonObject::fromVariantMap(linkObject->get("query").toMap());
                        QJsonArray docs = QJsonArray::fromVariantList(linkObject->get("documents").toList());
                        int r = tx->set(query, docs);
                        if (r != QDocCollection::success) {
                            errorString = tx->getLastError();
                        }
                        break;
                    }
                    }
                    if (txId == -1) {
                        int r = tx->writeTransaction();
                        if (r != QDocCollection::success) {
                            errorString = tx->getLastError();
                        }
                        delete tx;
                    }
                }
                break;
                }
            case QDocdbLinkObject::typeNewSnapshot:
            case QDocdbLinkObject::typeRevertToSnapshot:
            case QDocdbLinkObject::typeRemoveSnapshot: {
                if (snapshotName == "__CURRENT") {
                    errorString = "Invalid snapshot name: " + snapshotName;
                } else {
                    switch (intType) {
                    case QDocdbLinkObject::typeNewSnapshot: {
                        int r = coll->newSnapshot(snapshotName);
                        if (r != QDocCollection::success) {
                            errorString = coll->getLastError();
                        }
                        break;
                    }
                    case QDocdbLinkObject::typeRevertToSnapshot: {
                        int r = coll->revertToSnapshot(snapshotName);
                        if (r != QDocCollection::success) {
                            errorString = coll->getLastError();
                        }
                        break;
                    }
                    case QDocdbLinkObject::typeRemoveSnapshot: {
                        int r = coll->removeSnapshot(snapshotName);
                        if (r != QDocCollection::success) {
                            errorString = coll->getLastError();
                        }
                        break;
                    }
                    }
                }
                break;
            }
            case QDocdbLinkObject::typeObserve: {
                connect(coll, SIGNAL(observeQueryChanged(int,QJsonArray&)), this, SLOT(observeQueryChanged(int,QJsonArray&)), Qt::QueuedConnection);
                int observeId = coll->observe(QJsonObject::fromVariantMap(linkObject->get("query").toMap()),
                                              QJsonObject::fromVariantMap(linkObject->get("queryOptions").toMap()),
                                              linkObject->get("preferedId").toInt());
                this->observeClient[observeId] = client;
                this->observeCollection[observeId] = coll;
                replyObject = QDocdbLinkObject::newLinkObject(id, QDocdbLinkObject::typeObserveReply);
                replyObject->set("observeId", observeId);
                break;
            }
            case QDocdbLinkObject::typeUnobserve: {
                int observeId = linkObject->get("observeId").toInt();
                if (this->observeCollection.contains(observeId)) {
                    if (this->observeClient[observeId] == client) {
                        QDocCollection* ocoll = this->observeCollection[observeId];
                        ocoll->unobserve(observeId);
                        this->observeClient.remove(observeId);
                        this->observeCollection.remove(observeId);
                        disconnect(ocoll, SIGNAL(observeQueryChanged(int,QJsonArray&)), this, SLOT(observeQueryChanged(int,QJsonArray&)));
                    }
                }
                break;
            }
            default:
                break;
            }
        }
    }
    if (replyObject == NULL) {
        replyObject = QDocdbLinkObject::newLinkObject(id, QDocdbLinkObject::typeUnknown);
    }
    replyObject->set("error", errorString);
    client->send(replyObject);
    delete replyObject;
}

int QDocdbServer::getNextTransactionId() {
    int id = -1;
    while ((id < 0) || (this->transactions.contains(id))) {
        id = this->transactionId++;
        if (id < 0) {
            this->transactionId = 0;
        }
    }
    return id;
}

void QDocdbServer::observeQueryChanged(int observeId, QJsonArray& reply) {
    if (!this->observeClient.contains(observeId)) return;
    QDocdbLinkBase* client = this->observeClient[observeId];
    QDocdbLinkObject* replyObject = QDocdbLinkObject::newLinkObject(-1, QDocdbLinkObject::typeObserveData);
    replyObject->set("reply", reply.toVariantList());
    replyObject->set("observeId", observeId);
    client->send(replyObject);
    delete replyObject;
}

QDocdbServer::QDocdbServer(QString serverName, QDocdbCommonConfig* commonConfig, QString baseDir) {
    qRegisterMetaType<QJsonArray>("QJsonArray&");
    this->commonConfig = commonConfig;

    this->serverName = serverName;
    this->baseDir = baseDir + "/" + this->serverName;
    QDir dir(this->baseDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qInfo() << "Error: Can't create" << this->baseDir << endl;
            return;
        }
    }
    connect(&this->tcpServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
    this->valid = tcpServer.listen(QHostAddress::LocalHost);
    if (!this->valid) {
        qInfo() << "Error: Can't listen on " + this->serverName << endl;
    }
    const QString key("QDocdbServer/" + serverName);
    QSettings settings(QDOCDB_ORGANIZATION, QDOCDB_APPLICATION);
    QString host = tcpServer.serverAddress().toString() + ":" + QString::number(tcpServer.serverPort());
    settings.setValue(key, host);

    this->transactionId = 0;
}

QDocdbServer::~QDocdbServer() {
    for (QList<QDocdbLinkBase*>::iterator it = this->clients.begin(); it != this->clients.end();) {
        disconnect(*it, SIGNAL(clientRemoved()), this, SLOT(clientRemoved()));
        it = this->clients.erase(it);
    }
    for (QMap<QString, QDocDatabase*>::iterator it = this->databases.begin(); it != this->databases.end();) {
        QDocDatabase* db = it.value();
        delete db;
        it = this->databases.erase(it);
    }
    QSettings settings(QDOCDB_ORGANIZATION, QDOCDB_APPLICATION);
    settings.remove("QDocdbServer/" + this->serverName);
}
