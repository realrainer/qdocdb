
#include <QDataStream>

#include "qdocdblinkbase.h"

void QDocdbLinkBase::readyRead() {
    while (true) {
        qint64 bytesAvailable = this->socket->bytesAvailable();
        if (bytesAvailable == 0) return;
        if (this->rxDataLength == 0) {
            if (bytesAvailable < 4) return;
            this->rxData = this->socket->read(4);
            QDataStream ls(&this->rxData, QIODevice::ReadOnly);
            ls.setByteOrder(QDataStream::LittleEndian);
            ls >> this->rxDataLength;
        }
        if (this->rxDataLength != 0) {
            qint64 maxLength = this->rxDataLength - this->rxData.size() + 4;
            QByteArray data = this->socket->read(maxLength);
            this->rxData.append(data);
            if (this->rxDataLength == (this->rxData.size() - 4)) {
                QDocdbLinkObject* linkObject = new QDocdbLinkObject();
                linkObject->unmarshal(this->rxData);
                this->rxDataLength = 0;
                this->rxData.clear();
                if (!linkObject->isEmpty()) {
                    emit this->receive(linkObject);
                }
            }
        } else {
            return;
        }
    }
}

void QDocdbLinkBase::disconnected() {
    emit this->clientRemoved();
    this->deleteLater();
}

QTcpSocket* QDocdbLinkBase::getSocket() {
    return this->socket;
}

void QDocdbLinkBase::close() {
    this->socket->close();
}

void QDocdbLinkBase::send(QDocdbLinkObject* linkObject) {
    QByteArray data;
    linkObject->marshal(data);
    this->socket->write(data);
}

void QDocdbLinkBase::appendUnlockWait(QString, QDocdbLinkObject* linkObject) {
    this->unlockWait.enqueue(linkObject);
}

void QDocdbLinkBase::onUnlocked(QString) {
    int count = this->unlockWait.count();
    while (count--) {
        emit this->receive(this->unlockWait.dequeue());
    }
}

QDocdbLinkBase::QDocdbLinkBase(QTcpSocket* socket) {
    this->socket = socket;
    this->rxDataLength = 0;
    connect(this->socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(this->socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
}

QDocdbLinkBase::~QDocdbLinkBase() {
    disconnect(this->socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    disconnect(this->socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    while (this->unlockWait.count()) {
        delete this->unlockWait.dequeue();
    }
}

// ---

QDocdbLinkObject* QDocdbLinkObject::newLinkObject(int id, QDocdbLinkObject::LinkObjectType type) {
    QVariantMap fMap;
    fMap["id"] = id;
    fMap["func"] = QDOCDB_FUNCMAP[type];
    return new QDocdbLinkObject(fMap);
}

void QDocdbLinkObject::set(QString key, QVariant value) {
    this->fMap[key] = value;
}

QVariant QDocdbLinkObject::get(QString key) {
    return this->fMap[key];
}

void QDocdbLinkObject::marshal(QByteArray& data) {
    data.clear();
    QDataStream dataStream(&data, QIODevice::WriteOnly);
    QByteArray mapData;
    QDataStream mapDataStream(&mapData, QIODevice::WriteOnly);
    mapDataStream << this->fMap;
    dataStream.setByteOrder(QDataStream::LittleEndian);
    dataStream << mapData.size();
    data.append(mapData);
}

void QDocdbLinkObject::unmarshal(QByteArray &data) {
    QByteArray mapData = data.mid(4);
    QDataStream ds(&mapData, QIODevice::ReadOnly);
    ds >> this->fMap;
    if (!this->isEmpty()) {
        this->assignType();
    }
}

QVariantMap& QDocdbLinkObject::map() {
    return this->fMap;
}

bool QDocdbLinkObject::waitForDone() {
    this->signalSpy = new QSignalSpy(this, SIGNAL(done()));
    bool ok = this->signalSpy->wait(QDocdbLinkObject::waitForDoneTimeout);
    delete this->signalSpy;
    return ok;
}

void QDocdbLinkObject::assignType() {
    if (this->isEmpty()) return;
    this->type = QDOCDB_FUNCMAP.key(this->fMap["func"].toString());
}

bool QDocdbLinkObject::isEmpty() {
    return this->fMap.empty();
}

QDocdbLinkObject::LinkObjectType QDocdbLinkObject::getType() {
    return this->type;
}

QDocdbLinkObject::QDocdbLinkObject(QVariantMap& fMap) {
    this->fMap = fMap;
    this->assignType();
}

QDocdbLinkObject::QDocdbLinkObject() {
}

QDocdbLinkObject::~QDocdbLinkObject() {
}

