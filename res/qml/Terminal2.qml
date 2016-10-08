import QtQml 2.2
import QtQml.Models 2.2
import QtQuick 2.7
import QtQuick.Extras 1.4
import QtQuick.Dialogs 1.2
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
/*        id:background
        anchors.left:parent.left
        anchors.right:parent.right*/
   FontDialog {
        id: fontDialog
        onAccepted: {
        }
    }
    ColorDialog {
        id: colorDialog
        currentColor: "white"
    }
    MessageDialog {
        id: errorDialog
    }


//    Flickable {
//        id: flickable
//        flickableDirection:Flickable.VerticalFlick
//        anchors.fill: parent
//            property Item lastLine
//        }
    ListModel {
        id: history
        ListElement { folded: false; result: ""; input: "false"}
    }
    ListView {
        id: historyView
        Layout.fillWidth: true
        width: parent.width
        anchors.top:parent.top
        height:parent.height * 2./3
        model: history
        ScrollBar.vertical: ScrollBar{}
        clip: true
        spacing: 10
        currentIndex: Math.max(0,Math.min(input.historyPos,history.count-1))
        delegate : Column {
//            anchors.left: parent.left
            Text {
                font { family: "monospace" }
                color:"green"
                text: input
            }
            Rectangle { color: "grey"; width: historyView.width;height: 1}
            Text {
                id: res
                font { family: "monospace" }
                color:"blue"
                text: folded ? "[== CLICK TO UNFOLD ==]": result
                MouseArea {
                    anchors.fill:parent
                    onClicked: {
                        folded= !folded
                    }
                }
            }
            Rectangle { color: "black"; width: historyView.width;height: 2}
        }
    }
    Flickable {
        id:flickable
        anchors.top:historyView.bottom
        anchors.bottom:parent.bottom
        width:parent.width

        ScrollBar.vertical: ScrollBar{}

        TextArea.flickable: TextArea {
                id: input
                wrapMode:TextArea.Wrap
                focus: true
                selectByMouse: true
                persistentSelection: true
                background: Rectangle {
                    color: "black"
                    border { color: "white";width:1}
                }
                width: parent.width
    //            height: screen.height / 2
                color: colorDialog.currentColor
                property int historyPos: 0
                Keys.priority: Keys.AfterItem
                Keys.onPressed: {
                    if(event.modifiers & Qt.ControlModifier) {
                        if(event.key== Qt.Key_P) {
                            if(historyPos == history.count)
                                history.append({'input':text,'result':result,"folded":false})
                            else
                                history.set(historyPos, {
                                'input': input.text,
                                'result':history.get(historyPos).result,
                                'folded':false
                                })
                            historyPos-=1
                            input.text = history.get(historyPos).input
                            event.accepted = true;
                        }else if(event.key == Qt.Key_N) {
                        if(historyPos < history.count) {
                            history.set(historyPos, {'input': input.text,'result':history.get(historyPos).result,'folded':false})
                            historyPos += 1
                            if(historyPos == history.count)
                                input.text = ""
                            else
                                input.text = history.get(historyPos).input
                            event.accepted = true;
                        }
                        }else if(event.key == Qt.Key_Escape) {
                            if(historyPos < history.count) {
                                history.set(historyPos, {'input': input.text,'result':history.get(historyPos).result,'foled':false})
                                historyPos = history.count
                                input.text = ""
                                event.accepted = true;
                            }
                        }else if(event.key == Qt.Key_U) {
                            if(historyPos < history.count) {
                                history.set(historyPos, {'input': input.text,'result':history.get(historyPos).result,'foled':false})
                                historyPos = history.count
                            }
                            input.text = ""
                            event.accepted = true;
                        }
                    }else if(event.key== Qt.Key_Up) {
                        if(historyPos > 0){
                            if(historyPos == history.count)
                                history.append({'input':text,'result':result,'folded':false})
                            else
                                history.set(historyPos, {
                                'input': input.text,
                                'result':history.get(historyPos).result,
                                'folded':false
                                })
                            historyPos-=1
                            input.text = history.get(historyPos).input
                            event.accepted = true;
                        }
                    }else if(event.key == Qt.Key_Down) {
                        if(historyPos < history.count) {
                            history.set(historyPos, {'input': input.text,'result':history.get(historyPos).result,'foled':false})
                            historyPos += 1
                            if(historyPos == history.count)
                                input.text = ""
                            else
                                input.text = history.get(historyPos).input
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
    //                log(prompt.text + input.text)
                    try {
                        var result = String(Pretty.pretty(eval(text)))
                        history.append({'input':text,'result':result,'folded':false})
                        historyPos = history.count
                        input.text = ""
                        historyView.positionViewAtEnd();
                    } catch (any) {
                        result = String(Pretty.pretty(any))
                        history.append({'input':text,'result':result,'folded':false})
                        input.text = ""
                        historyView.positionViewAtEnd();
    //                    input.text = history.get(historyPos).input
                    }
    //                log(result)
    //                    prompt.anchors.top = background.lastLine.bottom;
                }
                function create(text) {
                    var result = ""
    //                log(prompt.text + input.text)
                    try {
                        var item = Qt.createQmlObject(text, screen,"line: " + historyPos)
                        result = String(Pretty.pretty(item))
    //                    result = String(eval(text))
                        history.append({'input':text,'result':result,'folded':false})
                        historyPos = historycount.
    //                    history.push(text)
                        input.text = ""
                    } catch (any) {
                        result = String(Pretty.pretty(any))
                        history.append({'input':text,'result':result,'folded':false})
                        historyPos = history.count

                    }
                }
                Component.onCompleted: {
    //                log("\n")
                    console.log("recreated!")
                }
        }
    }
    Menu {
        id: contextMenu
        MenuItem {
            text: qsTr("Copy")
            enabled: input.selectedText
            onTriggered:input.copy()
        }
        MenuItem {
            text: qsTr("Cut")
            enabled: input.selectedText
            onTriggered:input.cut()
        }
        MenuItem {
            text: qsTr("Paste")
            enabled: input.canPaste
            onTriggered:input.paste()
        }
//        MenuSeparator{}
        MenuItem {
            text: qsTr("Font...")
            onTriggered: fontDialog.open()
        }
        MenuItem {
            text: qsTr("Color...")
            onTriggered: colorDialog.open()
        }
    }
}
