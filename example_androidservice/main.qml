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
    title: qsTr("Andoid service test")
    QDocdbConnector {
        id: exchanger1
        url: "qdocdb://qdocdblocal/testdb?collection=coll&persistent=false"
        query: { "docType": "exchanger1" }
        onValueChanged: {
            console.log(JSON.stringify(exchanger1.value));
        }
        onValidChanged: {
            if (exchanger1.valid) {
                console.log(JSON.stringify(exchanger1.find({ "counter": { "$exists": true } })));
            }
        }
    }
    QDocdbConnector {
        id: exchanger2
        url: "qdocdb://qdocdblocal/testdb?collection=coll&persistent=false"
        query: { "docType": "exchanger2" }
    }
    Timer {
        interval: 1500
        running: true
        repeat: true
        onTriggered: {
            var value = exchanger2.value;
            if (value.length == 0) {
                value = [ { "docType": "exchanger2", "counter": 0 } ]
            } else {
                value[0].counter = value[0].counter + 1;
            }
            exchanger2.value = value;
        }
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
                    text: "exchanger1: " + exchanger1.value[0].counter;
                }
                Label {
                    text: "exchanger2: " + exchanger2.value[0].counter;
                }
                Label {
                    text: "valid: " + exchanger1.valid;
                }
                Label {
                    text: "lastError: " + exchanger1.err;
                }
            }
        }

        ScrollIndicator.vertical: ScrollIndicator { }
    }
}
