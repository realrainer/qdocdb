
#include <QTime>
#include <QDateTime>

#include "qdocdbcommonconfig.h"

QByteArray QDocIdGen::getId() {
    QByteArray id;
    this->counter++;
    unsigned int tstamp = QDateTime::currentSecsSinceEpoch() & 0xFFFFFFFF;
    id = QByteArray::number(tstamp, 16);
    id.append(QByteArray::number(this->counter, 16));
    return id;
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

QString QDocdbCommonConfig::getReadOnlyKey() {
    return this->readOnlyKey;
}

QDocdbCommonConfig::QDocdbCommonConfig(QDocIdGen *idGen, QString readOnlyKey) {
    if (idGen == NULL) {
        this->idGen = new QDocIdGen();
        this->isCustomIdGen = false;
    } else {
        this->idGen = idGen;
        this->isCustomIdGen = true;
    }
    this->readOnlyKey = readOnlyKey;
    this->readOnlyKeyValid = !readOnlyKey.isEmpty();
}

QDocdbCommonConfig::~QDocdbCommonConfig() {
    if (!this->isCustomIdGen) {
        delete this->idGen;
    }
}
