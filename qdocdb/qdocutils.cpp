
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

//---

QJsonValue QDocUtils::getJsonValueByPath(QJsonValue jsonValue, QString path) {
    if (path == "") {
        return jsonValue;
    }
    QString firstKey = path.mid(0, path.indexOf("."));
    QString nextPath;
    if (path.indexOf(".") == -1) {
        nextPath = "";
    } else {
        nextPath = path.mid(path.indexOf(".") + 1);
    }
    if (jsonValue.isObject()) {
        QJsonValue jsonValue1 = jsonValue.toObject().value(firstKey);
        return QDocUtils::getJsonValueByPath(jsonValue1, nextPath);
    }
    return QJsonValue(QJsonValue::Undefined);
}

QJsonArray QDocUtils::multiply(QJsonArray& a, QJsonArray& b) {
    QJsonArray res;
    for (QJsonArray::iterator ita = a.begin(); ita != a.end(); ita++) {
        bool find = false;
        for (QJsonArray::iterator itb = b.begin(); (itb != b.end()) && (!find); itb++) {
            if (*itb == *ita) find = true;
        }
        if (find) {
            res.append(*ita);
        }
    }
    return res;
}

bool QDocUtils::compare(QJsonValue &a, QJsonValue& b, QString oper) {
    if ((a.isDouble()) && (b.isDouble())) {
        if (oper == "$gt") {
            return a.toDouble() > b.toDouble();
        } else
        if (oper == "$gte") {
            return a.toDouble() >= b.toDouble();
        } else
        if (oper == "$lt") {
            return a.toDouble() < b.toDouble();
        } else
        if (oper == "$lte") {
            return a.toDouble() <= b.toDouble();
        } else {
            return false;
        }
    }
    if ((a.isString()) && (b.isString())) {
        if (oper == "$gt") {
            return a.toString() > b.toString();
        } else
        if (oper == "$gte") {
            return a.toString() >= b.toString();
        } else
        if (oper == "$lt") {
            return a.toString() < b.toString();
        } else
        if (oper == "$lte") {
            return a.toString() <= b.toString();
        } else {
            return false;
        }
    }
    return false;
}
