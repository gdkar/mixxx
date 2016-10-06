import org.mixxx.qml 0.1
import "./lib"
import QtQml 2.2
import QtQuick 2.7

Item {
    id: root
    property string group: "[Playlist]"
    property var controller:parent.controller
    property var bindings
    Pushy     { parent:root;prefix:"\u0090" + bindings.prev;item: "SelectPrevTrack";  }
    Pushy     { parent:root;prefix:"\u0090" + bindings.next;item: "SelectNextTrack";  }
    Pushy     { parent:root;prefix:"\u0090" + bindings.nextList;item: "SelectNextPlaylist"; }
    Pushy     { parent:root;prefix:"\u0090" + bindings.prevList;item: "SelectPrevPlaylist"; }
}
