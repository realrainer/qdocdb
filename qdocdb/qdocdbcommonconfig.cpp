
#include <QDebug>
#include <QTime>
#include <QDateTime>
#include <QUuid>

#include "qdocdbcommonconfig.h"

QByteArray QDocIdGen::getRandomBytes(int count) {
    QByteArray result;
    while (count--) {
        result.append(QByteArray::number(qrand() & 0xF, 16));
    }
    return result;
}

QByteArray QDocIdGen::getId() {
    QByteArray id;
    this->counter++;
    unsigned int tstamp = (QDateTime::currentMSecsSinceEpoch() / 1000) & 0xFFFFFFFF;
    id = QByteArray::number(tstamp, 16);
    id.append(QByteArray::number(this->counter, 16));
    return id;
}

QByteArray QDocIdGen::getUUID() {
    QUuid uuid = QUuid::createUuid();
    QByteArray result = uuid.toByteArray();
    result = result.mid(1, result.size() - 2);
    return result;
}

int QDocIdGen::getNextNumber() {
    return this->nextNumber++;
}

QDocIdGen::QDocIdGen() {
    qsrand(static_cast<quint64>(QTime::currentTime().msecsSinceStartOfDay()));
    this->counter = qrand();
    this->counter <<= 32;
    this->counter |= qrand() & 0xFFFFFFFF;
    this->nextNumber = 1;
}

QDocIdGen::~QDocIdGen() {
}

//---

QDocIdGen* QDocdbCommonConfig::getIdGen() {
    return this->idGen;
}

bool QDocdbCommonConfig::isReadOnlyKeyValid() {
    return this->readOnlyKeyValid;
}

bool QDocdbCommonConfig::isUUIDPathValid() {
    return this->UUIDPathValid;
}

QString QDocdbCommonConfig::getReadOnlyKey() {
    return this->readOnlyKey;
}

QString QDocdbCommonConfig::getUUIDPath() {
    return this->UUIDPath;
}

QDocdbCommonConfig::QDocdbCommonConfig(QDocIdGen *idGen, QString UUIDPath, QString readOnlyKey) {
    if (idGen == NULL) {
        this->idGen = new QDocIdGen();
        this->isCustomIdGen = false;
    } else {
        this->idGen = idGen;
        this->isCustomIdGen = true;
    }
    this->readOnlyKey = readOnlyKey;
    this->readOnlyKeyValid = !readOnlyKey.isEmpty();
    this->UUIDPath = UUIDPath;
    this->UUIDPathValid = !UUIDPath.isEmpty();
}

QDocdbCommonConfig::~QDocdbCommonConfig() {
    if (!this->isCustomIdGen) {
        delete this->idGen;
    }
}
