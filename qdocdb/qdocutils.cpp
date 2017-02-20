
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDataStream>
#include <QTime>
#include <QDateTime>

#include "qdocutils.h"

QByteArray QDocSerialize::marshal(QJsonValue value) {
    QByteArray bytes;
    bytes.append(char(QDocSerialize::typeValue));
    bytes.append(char(QDocSerialize::typeUndefined));
    if (value.isArray()) {
        bytes[0] = char(QDocSerialize::typeJsonDocument);
        bytes[1] = char(QDocSerialize::typeArray);
        QJsonDocument jsonDoc(value.toArray());
        bytes.append(jsonDoc.toBinaryData());
    } else
    if (value.isObject()) {
        bytes[0] = char(QDocSerialize::typeJsonDocument);
        bytes[1] = char(QDocSerialize::typeObject);
        QJsonDocument jsonDoc(value.toObject());
        bytes.append(jsonDoc.toBinaryData());
    } else {
        QByteArray valueData;
        QDataStream ds(&valueData, QIODevice::WriteOnly);
        if (value.isBool()) {
            bytes[1] = char(QDocSerialize::typeBool);
            ds << value.toBool();
        } else
        if (value.isDouble()) {
            bytes[1] == char(QDocSerialize::typeDouble);
            ds << value.toDouble();
        } else
        if (value.isString()) {
            bytes[1] = char(QDocSerialize::typeString);
            valueData = value.toString().toUtf8();
        } else
        if (value.isNull()) {
            bytes[1] = char(QDocSerialize::typeNull);
        }
        if (value.isUndefined()) {
            bytes[1] = char(QDocSerialize::typeUndefined);
        }
        bytes.append(valueData);
    }
    return bytes;
}

QJsonValue QDocSerialize::unmarshal(QByteArray& bytes) {
    if (bytes.size() < 2) return QJsonValue();
    if (bytes[0] == char(QDocSerialize::typeJsonDocument)) {
        QJsonDocument jsonDoc = QJsonDocument::fromBinaryData(bytes.mid(2));
        if (bytes[1] == char(QDocSerialize::typeArray)) {
            return QJsonValue(jsonDoc.array());
        } else
        if (bytes[1] == char(QDocSerialize::typeObject)) {
            return QJsonValue(jsonDoc.object());
        }
    } else {
        QByteArray valueData = bytes.mid(2);
        QDataStream ds(&valueData, QIODevice::ReadOnly);
        if (bytes[1] == char(QDocSerialize::typeUndefined)) {
            return QJsonValue(QJsonValue::Undefined);
        } else
        if (bytes[1] == char(QDocSerialize::typeNull)) {
            return QJsonValue();
        } else
        if (bytes[1] == char(QDocSerialize::typeBool)) {
            bool value;
            ds >> value;
            return QJsonValue(value);
        } else
        if (bytes[1] == char(QDocSerialize::typeDouble)) {
            double value;
            ds >> value;
            return QJsonValue(value);
        } else
        if (bytes[1] == char(QDocSerialize::typeString)) {
            QString value = QString::fromUtf8(valueData);
            return QJsonValue(value);
        }
    }
    return QJsonValue();
}

//---

QString QDocIdGen::getId() {
    QString id;
    this->counter++;
    unsigned int tstamp = QDateTime::currentSecsSinceEpoch() & 0xFFFFFFFF;
    id = QString::number(tstamp, 16);
    id += QString::number(this->counter, 16);
    return id;
}

QDocIdGen::QDocIdGen() {
    qsrand(static_cast<quint64>(QTime::currentTime().msecsSinceStartOfDay()));
    this->counter = qrand();
    this->counter <<= 32;
    this->counter |= qrand() & 0xFFFFFFFF;
}

QDocIdGen::~QDocIdGen() {

}

