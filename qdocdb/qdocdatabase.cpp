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

QDocCollection* QDocDatabase::collection(QString collName, bool inMemory) {
    QDocCollection* pColl = NULL;
    QString baseDir = this->baseDir + "/" + collName;
    QList<QDocCollection*>::iterator it;
    for (it = this->collList.begin(); (it != this->collList.end()) && (pColl == NULL); it++) {
        if ((*it)->getBaseDir() == baseDir) {
            pColl = *it;
        }
    }
    if (pColl == NULL) {
        pColl = QDocCollection::open(baseDir, &this->idGen, inMemory);
        if (pColl != NULL) {
            this->collList.append(pColl);           
        }
    }
    return pColl;
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
