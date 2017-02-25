# qdocdb
Leveldb based embedded json document database released in Qt. May be used in QML projects on any platforms (Windows, Linux, Android ...)
This project is not ready for production and made  'just for fun'

### Required:
Only Qt is required to compile

### Supported:
* Databases and collections,
* Indexes, but may only used on top of query,
* Query operators $and, $or, $in, $exist, $lt, $lte, $gt, $gte, $not,
* Observable queries
* Transactions
* Read-only snapshots

### Example usage:
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
    }
}

```
* Result of query is observable and can be used as model from testCollection.value.

![Alt text](/qdocdb.png?raw=true "Example usage")

As example, see main.qml file.

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
* Create new snapshot:
```
testCollection.newSnapshot("test_snapshot");
```
