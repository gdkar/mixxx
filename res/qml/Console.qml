import org.mixxx.qml 0.1
import QtQml 2.2
import QtQml.Models 2.2
import QtQuick 2.7
import QtQuick.Extras 1.4
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0

Rectangle {
    id: screen
    anchors.fill: parent
    color: "black"
    property Item lastLine
    Component {
        id: outputLineComponent
        Text {
            id: text
            anchors.left: parent.left
            anchors.right: parent.right
            color: "white"
            font { family: "monospace" }
        }
    }
    Text {
        id: prompt
        anchors.left: parent.left
        text: "> "
        color: "blue"
        font { family: "monospace" }
    }
    TextInput {
        id: input
        focus: true
        anchors.left: prompt.right
        anchors.right: parent.right
        anchors.top: prompt.top
        color: "white"
        font: prompt.font
        onAccepted: run(text)
        function run(text) {
            var result = ""
            try {
                result = String(eval(text))
            }
            catch (any) {
                result = String(any)
            }
            log(prompt.text + input.text)
            log(result)
            prompt.y = lastLine.y  + lastLine.height
            input.text = ""
        }
        function log(text) {
            var outputLine = outputLineComponent.createObject(parent)
            outputLine.y = 0
            if(lastLine != null)
                outputLine.y = lastLine.y + lastLine.height
            outputLine.text = text
            lastLine = outputLine
        }
    }
}
