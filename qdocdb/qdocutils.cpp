
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDataStream>

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

QByteArray QDocSerialize::marshalIndexes(QHash<QString, QString> value) {
    QByteArray bytes;
    bytes.append(char(QDocSerialize::typeIndexes));
    bytes.append('\x0');
    QByteArray valueData;
    QDataStream ds(&valueData, QIODevice::WriteOnly);
    ds << value;
    bytes.append(valueData);
    return bytes;
}

QHash<QString, QString> QDocSerialize::unmarshalIndexes(QByteArray &bytes) {
    QHash<QString, QString> value;
    if ((bytes.size() < 2) || (bytes[0] != char(QDocSerialize::typeIndexes))) return value;
    QByteArray valueData = bytes.mid(2);
    QDataStream ds(&valueData, QIODevice::ReadOnly);
    ds >> value;
    return value;
}

QByteArray QDocSerialize::marshalIndexValue(QList<QByteArray> value) {
    QByteArray bytes;
    bytes.append(char(QDocSerialize::typeIndexValue));
    bytes.append('\x0');
    QByteArray valueData;
    QDataStream ds(&valueData, QIODevice::WriteOnly);
    ds << value;
    bytes.append(valueData);
    return bytes;
}

QList<QByteArray> QDocSerialize::unmarshalIndexValue(QByteArray &bytes) {
    QList<QByteArray> value;
    if ((bytes.size() < 2) || (bytes[0] != char(QDocSerialize::typeIndexValue))) return value;
    QByteArray valueData = bytes.mid(2);
    QDataStream ds(&valueData, QIODevice::ReadOnly);
    ds >> value;
    return value;
}

QByteArray QDocSerialize::marshalSnapshotsValue(QList<QString> value) {
    QByteArray bytes;
    bytes.append(char(QDocSerialize::typeSnapshotsValue));
    bytes.append('\x0');
    QByteArray valueData;
    QDataStream ds(&valueData, QIODevice::WriteOnly);
    ds << value;
    bytes.append(valueData);
    return bytes;
}

QList<QString> QDocSerialize::unmarshalSnapshotsValue(QByteArray &bytes) {
    QList<QString> value;
    if ((bytes.size() < 2) || (bytes[0] != char(QDocSerialize::typeSnapshotsValue))) return value;
    QByteArray valueData = bytes.mid(2);
    QDataStream ds(&valueData, QIODevice::ReadOnly);
    ds >> value;
    return value;
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
    } else
    if (jsonValue.isArray()) {
        bool ok = false;
        int index = firstKey.toInt(&ok, 10);
        if (ok) {
            QJsonValue jsonValue1 = jsonValue.toArray().at(index);
            return QDocUtils::getJsonValueByPath(jsonValue1, nextPath);
        }
    }
    return QJsonValue(QJsonValue::Undefined);
}

QList<QByteArray> QDocUtils::multiply(QList<QByteArray> a, QList<QByteArray>& b) {
    for (QList<QByteArray>::iterator ita = a.begin(); ita != a.end(); ita++) {
        if (!b.contains(*ita)) {
            a.erase(ita);
        }
    }
    return a;
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

