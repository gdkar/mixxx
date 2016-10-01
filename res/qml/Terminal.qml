import QtQml 2.2
import QtQml.Models 2.2
import QtQuick 2.7
import QtQuick.Extras 1.4
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0

import org.mixxx.qml 0.1

import "../js/pretty.js" as Pretty
//import org.mixxx.qml.ladspa 0.1
ApplicationWindow {
    id: screen
    width: 540
    height: 300
    visible: true
    Rectangle {
        id:background
        anchors.left:parent.left
        anchors.right:parent.right
        property int historyPos: 0
        property var history: new Array(0)
        color: "black"
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
//            property Item lastLine
            Text{
                id: output
                font { family: "monospace" }
                color: "green"
                anchors.left:parent.left
                anchors.right:parent.right
                anchors.top: parent.top

            }
            Text {
                id: prompt
                anchors.left: parent.left
                anchors.top: output.bottom
                text: "> "
                color: "blue"
                font { family: "monospace" }
            }
            TextEdit{
                id: input
                focus: true
                persistentSelection: true
                anchors.left:prompt.right
                anchors.right:prompt.left
                anchors.top:prompt.top
                color: "white"
                font: prompt.font
                property alias historyPos: background.historyPos
                property alias history: background.history
                Keys.priority: Keys.AfterItem
                Keys.onPressed: {
                    if(event.modifiers & Qt.ControlModifier) {
                        if(event.key== Qt.Key_P) {
                            if(historyPos > 0){
                                if(historyPos == history.length)
                                    history.push(input.text)
                                historyPos-=1
                                input.text = history[historyPos]
                                event.accepted = true;
                            }
                        }else if(event.key == Qt.Key_N) {
                            if(historyPos < history.length) {
                                history[historyPos] = input.text
                                historyPos += 1
                                if(historyPos == history.length)
                                    input.text = ""
                                else
                                    input.text = history[historyPos]
                                event.accepted = true;
                            }
                        }else if(event.key == Qt.Key_Escape) {
                            if(historyPos < history.length) {
                                history[historyPos] = input.text;
                                historyPos = history.length
                                input.text = ""
                                event.accepted = true;
                            }
                        }else if(event.key == Qt.Key_U) {
                            if(historyPos < history.length) {
                                history[historyPos] = input.text;
                                historyPos = history.length
                            }
                            input.text = ""
                            event.accepted = true;
                        }
                    }else if(event.key== Qt.Key_Up) {
                        if(historyPos > 0){
                            if(historyPos == history.length)
                                history.push(input.text)
                            historyPos-=1
                            input.text = history[historyPos]
                            event.accepted = true;
                        }
                    }else if(event.key == Qt.Key_Down) {
                        if(historyPos < history.length) {
                            history[historyPos] = input.text
                            historyPos += 1
                            if(historyPos == history.length)
                                input.text = ""
                            else
                                input.text = history[historyPos]
                            event.accepted = true;
                        }
                    }
                }
                Keys.onReturnPressed: {
                    if(event.modifiers & Qt.ControlModifier && event.modifiers & Qt.ShiftModifier) {
                        event.accepted = true
                        create(text);
                    }else if(event.modifiers & Qt.ControlModifier) {
                        event.accepted=true
                        run(text)
                    }
                }
    //            onAccepted: run(text)
                function run(text) {
                    var result = ""
                    log(prompt.text + input.text)
                    try {
                        result = String(Pretty.pretty(eval(text)))
                        history.push(text)
                        historyPos = history.length
                        input.text = ""
                    } catch (any) {
                        result = String(Pretty.pretty(any))
                    }
                    log(result)
//                    prompt.anchors.top = background.lastLine.bottom;
                }
                function create(text) {
                    var result = ""
                    log(prompt.text + input.text)
                    try {
                        var item = Qt.createQmlObject(text, background,"line: " + historyPos)
                        result = String(Pretty.pretty(item))
    //                    result = String(eval(text))
                        history.push(text)
                        input.text = ""
                    } catch (any) {
                        result = String(Pretty.pretty(any))
                    }
                    log(result)
//                    prompt.anchors.top = background.lastLine.bottom;
                }

                function log(text) {
//                    var outputLine = outputLineComponent.createObject(parent)
//                    outputLine.y = 0
//                    if(background.lastLine != null)
                    output.text += "\n\n" + text
//                        outputLine.anchors.top = background.lastLine.bottom
//                        outputLine.y = background.lastLine.y + background.lastLine.height
//                    outputLine.text = text
//                    background.lastLine = outputLine
                }
                Component.onCompleted: {
                    log("\n")
                    console.log("recreated!")
                }
            }
    }
}
