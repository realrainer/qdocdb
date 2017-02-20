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
        database: "testdb"
        collection: "coll1"
        query: {
            "name": {
                "$exists": true
            },
            "properties.age": { "$gte": parseInt(ageQueryTextField.text) }
        }
    }
    QDocdbConnector {
        id: coll2
        database: "testdb"
        collection: "coll1"
        query: { }
    }
    Component.onCompleted: {
        coll1.createIndex("name", "IDX_NAME")
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
                                text: modelData.name + ", age: " + modelData.properties.age
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
                    Label {
                        text: "Query: age >= "
                        font.pixelSize: 14
                        font.italic: true
                    }
                    TextField {
                        id: ageQueryTextField
                        text: "30"
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
                                text: modelData.name + ", age: " + modelData.properties.age
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
                    columns: 2
                    Label { text: "Name: " }
                    TextField {
                        id: nameTextField
                    }
                    Label { text: "Age: " }
                    TextField {
                        id: ageTextField
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
                                            "age": parseInt(ageTextField.text)
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
                }
            }
        }

        ScrollIndicator.vertical: ScrollIndicator { }
    }
}
