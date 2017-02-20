#include <QDir>
#include <QDebug>
#include <QObject>
#include "qdocdatabase.h"

int QDocDatabase::open(QString baseDir) {
    QDir dir(baseDir);
    if (!dir.exists()) {
        this->isOpened = dir.mkpath(".");
    } else {
        this->isOpened = true;
    }
    this->baseDir = dir.canonicalPath();

    if (!this->isOpened) {
        qDebug() << "Error: Can't create " << baseDir << endl;
        return QDocDatabase::errorDatabase;
    }
    return QDocDatabase::success;
}

QDocCollection* QDocDatabase::collection(QString collName) {
    QDocCollection* pColl = NULL;
    QString baseDir = this->baseDir + "/" + collName;
    QList<QDocCollection*>::iterator it;
    for (it = this->collList.begin(); (it != this->collList.end()) && (pColl == NULL); it++) {
        if ((*it)->getBaseDir() == baseDir) {
            pColl = *it;
        }
    }
    if (pColl == NULL) {
        pColl = QDocCollection::open(baseDir, &this->idGen);
        if (pColl != NULL) {
            this->collList.append(pColl);
            connect(pColl, SIGNAL(observeQueryChanged(int, QJsonArray&)), this, SLOT(observeQueryChanged(int, QJsonArray&)), Qt::QueuedConnection);
        }
    }
    return pColl;
}

int QDocDatabase::observe(QDocCollection *pColl, QJsonObject &query, QObject* pObject) {
    int observeId = pColl->observe(query);
    if (observeId != -1) {
        this->observeHash[observeId] = pObject;
        return QDocDatabase::errorDatabase;
    }
    return QDocDatabase::success;
}

int QDocDatabase::unobserve(QDocCollection *pColl, QObject *pObject) {
    foreach(int observeId, this->observeHash.keys()) {
        if (this->observeHash[observeId] == pObject) {
            pColl->unobserve(observeId);
            this->observeHash.remove(observeId);
        }
    }
    return QDocDatabase::success;
}

void QDocDatabase::observeQueryChanged(int observeId, QJsonArray& reply) {
    if (this->observeHash.contains(observeId)) {
        QObject* pObject = this->observeHash[observeId];
        QMetaObject::invokeMethod(pObject, "observeQueryChanged", Q_ARG(QJsonArray, reply));
    }
}

QDocDatabase::QDocDatabase() {
    this->isOpened = false;
    this->collList.clear();
}

QDocDatabase::~QDocDatabase() {
    while (collList.count()) {
        QDocCollection* pColl = collList.takeFirst();
        if (pColl != NULL) {
            delete pColl;
        }
    }
}
