import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15


Popup {
    id: root
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    padding: 18
    background: Rectangle {
        radius: 16
        color: "#0F1020"
        border.color: "#2a7cff"
        border.width: 2
    }


    property int candidateScore: 0
    property bool browsing: false


    function consider(score) {
        candidateScore = score
        browsing = false
        if (highScoreManager.qualifies(score)) {
            open()
        } else {
            openForBrowsing()
        }
    }


    function openForBrowsing() {
        browsing = true
        candidateScore = 0
        open()
    }


    contentItem: ColumnLayout {
        spacing: 12
        width: 520


        Label {
            Layout.alignment: Qt.AlignHCenter
            text: browsing ? "Highscores" : "New Highscore!"
            font.pixelSize: 28
            color: "#9ad7ff"
        }


        // Entry form if not browsing
        RowLayout {
            visible: !browsing
            spacing: 10
            Label { text: "Your score:"; color: "#cde7ff" }
            Label { text: candidateScore; color: "#7dfcff"; font.bold: true }
        }


        RowLayout {
            visible: !browsing
            spacing: 10
            Label { text: "Name:"; color: "#cde7ff" }
            TextField { id: nameField; Layout.fillWidth: true; placeholderText: "Type your name" }
            Button {
                text: "Save"
                onClicked: {
                    if (nameField.text.length === 0) return
                    highScoreManager.submitScore(nameField.text, candidateScore)
                    browsing = true
                    candidateScore = 0
                    scoresView.model = highScoreManager.loadScores()
                }
            }
        }


        // Highscore list
        ListView {
            id: scoresView
            Layout.fillWidth: true
            Layout.preferredHeight: 300
            clip: true
            model: highScoreManager.loadScores()
            delegate: Rectangle {
                height: 34
                color: index % 2 ? "#12142a" : "#0e1125"
                width: parent.width
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    Label { text: (index+1) + "."; color: "#7dfcff"; width: 28 }
                    Label { text: modelData.name; color: "#d0a2ff"; Layout.fillWidth: true }
                    Label { text: modelData.score; color: "#98ffce" }
                }
            }
            ScrollBar.vertical: ScrollBar {}
        }


        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 8
            Button { text: "Close"; onClicked: root.close() }
        }
    }
}
