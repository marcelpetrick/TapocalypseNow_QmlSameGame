import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root

    property int candidateScore: 0
    property bool browsing: true
    property var highScoreService: null
    // Base size for text, set by the window so the dialog scales with the screen.
    property real unit: 18
    // True while a finished run can still be saved; guarantees one submission per run.
    readonly property bool canSubmit: !browsing && candidateScore > 0
    readonly property string trimmedName: nameField.text.trim()

    function consider(score) {
        candidateScore = score;
        browsing = !(highScoreService && highScoreService.qualifies(score));
        nameField.text = "";
        open();
        if (!browsing)
            nameField.forceActiveFocus();

    }

    function openForBrowsing() {
        browsing = true;
        candidateScore = 0;
        open();
    }

    function submit() {
        if (!canSubmit || trimmedName.length === 0)
            return ;

        const saved = highScoreService.submitScore(trimmedName, candidateScore);
        candidateScore = 0;
        browsing = true;
        nameField.text = "";
        statusLabel.text = saved ? "Saved. Can you beat it?" : "Could not write the highscore file.";
    }

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    padding: unit
    width: Math.min(parent ? parent.width - 2 * unit : 560, unit * 30)
    height: Math.min(parent ? parent.height - 2 * unit : 640, implicitHeight)
    onClosed: statusLabel.text = ""

    background: Rectangle {
        radius: root.unit
        color: "#0F1020"
        border.color: "#2a7cff"
        border.width: 2
    }

    contentItem: ColumnLayout {
        spacing: root.unit * 0.6

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: root.browsing ? "Highscores" : "New Highscore!"
            font.pixelSize: root.unit * 1.6
            color: "#9ad7ff"
        }

        Label {
            Layout.fillWidth: true
            visible: root.browsing && statusLabel.text.length === 0 && scoresView.count === 0
            horizontalAlignment: Text.AlignHCenter
            text: "No scores yet. Clear a group of three to get started."
            wrapMode: Text.Wrap
            font.pixelSize: root.unit
            color: "#cde7ff"
        }

        Label {
            id: statusLabel

            Layout.fillWidth: true
            visible: text.length > 0
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.pixelSize: root.unit
            color: "#98ffce"
        }

        // Entry form for a qualifying finished run
        RowLayout {
            visible: !root.browsing
            spacing: root.unit * 0.5

            Label {
                text: "Your score:"
                font.pixelSize: root.unit
                color: "#cde7ff"
            }

            Label {
                text: root.candidateScore
                font.pixelSize: root.unit
                color: "#7dfcff"
                font.bold: true
            }

        }

        RowLayout {
            visible: !root.browsing
            spacing: root.unit * 0.5

            TextField {
                id: nameField

                objectName: "nameField"
                Layout.fillWidth: true
                placeholderText: "Your name"
                maximumLength: 20
                font.pixelSize: root.unit
                onAccepted: root.submit()
            }

            Button {
                objectName: "saveButton"
                text: "Save"
                font.pixelSize: root.unit
                enabled: root.canSubmit && root.trimmedName.length > 0
                onClicked: root.submit()
            }

        }

        // Highscore list, bound to the notifying C++ property so it never goes stale
        ListView {
            id: scoresView

            objectName: "scoresView"
            Layout.fillWidth: true
            Layout.fillHeight: true
            // Grow with the entries (up to the full top ten) instead of reserving empty space.
            Layout.preferredHeight: count * root.unit * 2
            visible: count > 0
            clip: true
            model: root.highScoreService ? root.highScoreService.scores : []

            delegate: Rectangle {
                height: root.unit * 2
                width: ListView.view.width
                color: index % 2 ? "#12142a" : "#0e1125"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: root.unit * 0.5
                    anchors.rightMargin: root.unit * 0.5

                    Label {
                        text: (index + 1) + "."
                        font.pixelSize: root.unit
                        color: "#7dfcff"
                        Layout.preferredWidth: root.unit * 2
                    }

                    Label {
                        text: modelData.name
                        font.pixelSize: root.unit
                        color: "#d0a2ff"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: modelData.score
                        font.pixelSize: root.unit
                        color: "#98ffce"
                    }

                }

            }

            ScrollBar.vertical: ScrollBar {
            }

        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: "Close"
            font.pixelSize: root.unit
            onClicked: root.close()
        }

    }

}
