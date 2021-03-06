import QtQuick 2.7
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.0
import QtGraphicalEffects 1.0

import com.example.qdocdb.classes 1.0

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 360
    height: 520
    title: qsTr("Document database test")
    QDocdbConnector {
        id: coll1
        url: "qdocdb://qdocdblocal/testdb?collection=coll1"
        query: {
            "name": {
                "$exists": true
            },
            "properties.age": { "$gte": parseInt(ageQueryTextField.text) },
            "$not": {
                "properties.gender": "male"
            }
        }
        queryOptions: {
            "$orderBy": {
                "properties.age": 1
            }
        }
    }
    QDocdbConnector {
        id: coll2
        url: "qdocdb://qdocdblocal/testdb?collection=coll1"
        query: { }
    }
    QDocdbConnector {
        id: coll3
        url: "qdocdb://qdocdblocal/testdb?collection=coll3&persistent=false"
        query: { }
    }
    Component.onCompleted: {
        coll1.createIndex("name", "IDX_NAME");
        coll1.createIndex("properties.gender", "IDX_GENDER");
    }

    Flickable {
        id: flickable
        contentHeight: pane.height
        anchors.fill: parent

        Pane {
            id: pane
            width: flickable.width

            ColumnLayout {
                Layout.fillWidth: true

                Label {
                    text: "Database content:"
                    font.pixelSize: 14
                    font.italic: true
                }
                RowLayout {
                    ColumnLayout {
                        Repeater {
                            id: dbContentRepeater
                            property string checkedDocId
                            model: coll2.value
                            delegate: RadioButton {
                                text: modelData.name + ", " + modelData.properties.gender + ", age: " + modelData.properties.age
                                onCheckedChanged: {
                                    if (checked) {
                                        dbContentRepeater.checkedDocId = modelData._id;
                                    }
                                }
                            }
                        }
                        Label {
                            text: "(empty)"
                            visible: coll2.value.length == 0
                        }
                    }
                }
                RowLayout {
                    Button {
                        enabled: dbContentRepeater.checkedDocId != ""
                        text: "Remove"
                        onClicked: {
                            if (dbContentRepeater.checkedDocId != "") {
                                coll2.removeId(dbContentRepeater.checkedDocId);
                                dbContentRepeater.checkedDocId = ""
                            }
                        }
                    }
                    Button {
                        enabled: coll2.value.length != 0
                        text: "Clear"
                        onClicked: {
                            coll2.removeQueryResults();
                        }
                    }
                }
                RowLayout {
                    Button {
                        text: "NEW SNAP"
                        onClicked: {
                            coll2.newSnapshot("test_snapshot");
                        }
                    }
                    Button {
                        text: "REVERT SNAP"
                        onClicked: {
                            coll2.revertToSnapshot("test_snapshot");
                        }
                    }
                    Button {
                        text: "REMOVE SNAP"
                        onClicked: {
                            coll2.removeSnapshot("test_snapshot");
                        }
                    }
                }
                RowLayout {
                    Label {
                        text: "Query: age >= "
                        font.pixelSize: 14
                        font.italic: true
                    }
                    TextField {
                        id: ageQueryTextField
                        text: "30"
                    }
                    Label {
                        text: " and gender!=male"
                        font.pixelSize: 14
                        font.italic: true
                    }
                }
                Label {
                    text: "Query result:"
                    font.pixelSize: 14
                    font.italic: true
                }
                RowLayout {
                    ColumnLayout {
                        Repeater {
                            model: coll1.value
                            delegate: Label {
                                text: modelData.name + ", " + modelData.properties.gender + ", age: " + modelData.properties.age
                            }
                        }
                    }
                }
                Label {}
                Label {
                    text: "New document:"
                    font.pixelSize: 14
                    font.italic: true
                }
                GridLayout {
                    columns: 4
                    Label { text: "Name: " }
                    TextField {
                        id: nameTextField
                    }
                    Label { text: "Age: " }
                    TextField {
                        id: ageTextField
                    }
                    Label { text: "Gender:" }
                    ComboBox {
                        id: genderComboBox
                        model: [ "male", "female" ]
                    }
                }
                RowLayout {
                    Button {
                        text: "Insert"
                        onClicked: {
                            var erroredData = true;
                            if ((nameTextField.text != "") && (ageTextField != "")) {
                                var age = parseInt(ageTextField.text);
                                if (age > 0) {
                                    erroredData = false;
                                    coll2.insert({
                                        "name": nameTextField.text,
                                        "properties": {
                                            "age": parseInt(ageTextField.text),
                                            "gender": genderComboBox.currentText
                                        }
                                    });
                                }
                                nameTextField.text = "";
                                ageTextField.text = "";
                            }
                            if (erroredData) {

                            }
                        }
                    }
                    Button {
                        text: "Test"
                        onClicked: {
                            var reply = coll2.find({ "_id": { "$exists": true }});
                            console.log(JSON.stringify(reply));
                        }
                    }
                }
                RowLayout {
                    Label {
                        text: coll1.err;
                    }
                }
            }
        }

        ScrollIndicator.vertical: ScrollIndicator { }
    }
}
