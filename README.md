# qdocdb
Leveldb based embedded json document database released in Qt. May be used in QML projects on any platforms (Windows, Linux, Android ...)
This project is not ready for production and made  'just for fun'.
Leveldb qt project was get from https://github.com/paulovap/qtleveldb/tree/master/src/3rdparty/leveldb

### Required:
Qt 5.8

### Supported:
* Databases and collections,
* Indexes, but may only used on top of query,
* Query operators $and, $or, $in, $exist, $lt, $lte, $gt, $gte, $not,
* Query options $orderBy, $limit,
* Observable queries
* Transactions
* Read-only snapshots

### Example usage:
* Create directory qdocdb in your qml project and clone this repository to created directory:
```
git clone https://github.com/realrainer/qdocdb/
```
* Include qdocdb.pri in your .pro file:
```
include(qdocdb/qdocdb.pri)
```
* In main.cpp register qml type:
```
qmlRegisterType<QDocdbConnector>("com.example.qdocdb.classes", 1, 0, "QDocdbConnector");
```
* In qml file:
```
import com.example.qdocdb.classes 1.0

QDocdbConnector {
    id: testCollection
    database: "testdb"
    collection: "test"
    query: {
        "name": {
            "$exists": true
        },
        "properties.age": {
            "$gte": 30
        },
        $not: {
            "properties.gender": "male"
        }
    },
    queryOptions: {
        "$orderBy": {
            "name": 1
        },
        $limit: 10
    }
}
```
* Result of query is observable and can be used as model from testCollection.value.

![Alt text](/qdocdb.png?raw=true "Example usage")

As example, see example/main.qml file.

#### Other examples:

* Insert document:
```
testCollection.insert({
    "name": "John",
    "properties": {
        "age": 34,
        "gender": "male"
    }
});
```
* Remove document:
```
testCollection.remove({
    "name": "John"
});
```
* Create index:
```
testCollection.createIndex("name", "IDX_NAME");
```
* Snapshot functions:
```
testCollection.newSnapshot("test_snapshot");
testCollection.revertToSnapshot("test_snapshot");
or
testCollection.removeSnapshot("test_snapshot");
```
* 