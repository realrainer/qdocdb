#ifndef QDOCUTILS_H
#define QDOCUTILS_H

#include <QJsonValue>
#include <QByteArray>
#include <QString>

class QDocSerialize {
    enum levelType {
        typeValue = 0x0,
        typeJsonDocument = 0x1,
        typeIndexes = 0x2,
        typeIndexValue = 0x3,
        typeSnapshotsValue = 0x4
    };
    enum Type {
        typeUndefined = 0x0,
        typeNull = 0x1,
        typeBool = 0x2,
        typeDouble = 0x3,
        typeString = 0x4,
        typeArray = 0x5,
        typeObject = 0x6
    };

public:
    static QByteArray marshal(QJsonValue value);
    static QJsonValue unmarshal(QByteArray& bytes);
    static QByteArray marshalIndexes(QHash<QString, QString> value);
    static QHash<QString, QString> unmarshalIndexes(QByteArray &bytes);
    static QByteArray marshalIndexValue(QList<QByteArray> value);
    static QList<QByteArray> unmarshalIndexValue(QByteArray &bytes);
    static QByteArray marshalSnapshotsValue(QList<QString> value);
    static QList<QString> unmarshalSnapshotsValue(QByteArray &bytes);
};

class QDocIdGen {
    quint64 counter;
public:
    QByteArray getId();
    QDocIdGen();
    ~QDocIdGen();
};

class QDocUtils {
public:
    static QJsonValue getJsonValueByPath(QJsonValue jsonValue, QString path);
    static QList<QByteArray> multiply(QList<QByteArray> a, QList<QByteArray>& b);
    static bool compare(QJsonValue& a, QJsonValue& b, QString oper);
};

#endif // QDOCUTILS_H
