#include <QFile>
#include <QSettings>
#include <QtEndian>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#ifdef TAPOCALYPSE_HAS_SOUND
#include <QAudioDevice>
#include <QMediaDevices>
#include <QSoundEffect>
#endif

#include "SoundEffects.h"

class SoundEffectsTest : public QObject {
    Q_OBJECT

   private slots:
    void init();
    void popCue_growsWithGroupSize_data();
    void popCue_growsWithGroupSize();
    void everyCue_hasDistinctName();
    void play_emitsCueUnlessMuted();
    void muted_persistsAcrossInstances();
    void muted_restoresStoredValue();
    void cues_areCompiledInAsPcmWav();
    void cues_loadOnTheAudioDevice();

   private:
    QString settingsPath() const { return m_dir.filePath(QStringLiteral("settings.ini")); }

    QTemporaryDir m_dir;
};

void SoundEffectsTest::init() {
    QVERIFY(m_dir.isValid());
    QFile::remove(settingsPath());
}

void SoundEffectsTest::popCue_growsWithGroupSize_data() {
    QTest::addColumn<int>("groupSize");
    QTest::addColumn<QString>("cue");
    QTest::newRow("3") << 3 << "pop1";
    QTest::newRow("4") << 4 << "pop1";
    QTest::newRow("5") << 5 << "pop2";
    QTest::newRow("7") << 7 << "pop2";
    QTest::newRow("8") << 8 << "pop3";
    QTest::newRow("11") << 11 << "pop3";
    QTest::newRow("12") << 12 << "pop4";
    QTest::newRow("160") << 160 << "pop4";
}

void SoundEffectsTest::popCue_growsWithGroupSize() {
    QFETCH(int, groupSize);
    QFETCH(QString, cue);
    QCOMPARE(SoundEffects::cueName(SoundEffects::popCue(groupSize)), cue);
}

void SoundEffectsTest::everyCue_hasDistinctName() {
    QSet<QString> names;
    for (int index = 0; index < SoundEffects::kCueCount; ++index) {
        const QString name = SoundEffects::cueName(static_cast<SoundEffects::Cue>(index));
        QVERIFY(!name.isEmpty());
        names.insert(name);
    }
    QCOMPARE(names.size(), SoundEffects::kCueCount);
}

void SoundEffectsTest::play_emitsCueUnlessMuted() {
    SoundEffects sounds(settingsPath());
    sounds.setVolume(0);
    QSignalSpy played(&sounds, &SoundEffects::cuePlayed);

    sounds.playPop(9);
    sounds.playReject();
    sounds.playLand();
    sounds.playDeal();
    sounds.playGameOver();
    sounds.playHighscore();
    QStringList names;
    for (const auto& call : played) names << call.constFirst().toString();
    QCOMPARE(names, QStringList({"pop3", "reject", "land", "deal", "gameover", "highscore"}));

    sounds.setMuted(true);
    played.clear();
    sounds.playPop(3);
    sounds.playHighscore();
    QCOMPARE(played.count(), 0);
}

void SoundEffectsTest::muted_persistsAcrossInstances() {
    {
        SoundEffects sounds(settingsPath());
        QVERIFY(!sounds.muted());
        QSignalSpy changed(&sounds, &SoundEffects::mutedChanged);
        sounds.setMuted(true);
        sounds.setMuted(true);
        QCOMPARE(changed.count(), 1);
    }
    SoundEffects restarted(settingsPath());
    QVERIFY(restarted.muted());
    restarted.setMuted(false);
    QVERIFY(!SoundEffects(settingsPath()).muted());
}

void SoundEffectsTest::muted_restoresStoredValue() {
    {
        QSettings settings(settingsPath(), QSettings::IniFormat);
        settings.setValue(QStringLiteral("sound/muted"), true);
    }
    QVERIFY(SoundEffects(settingsPath()).muted());
}

void SoundEffectsTest::cues_areCompiledInAsPcmWav() {
#ifndef TAPOCALYPSE_HAS_SOUND
    QVERIFY(!SoundEffects(settingsPath()).available());
    QSKIP("built without Qt Multimedia");
#else
    // Every generated cue must be in the binary as 16-bit PCM mono WAV, independent of the audio hardware.
    for (int index = 0; index < SoundEffects::kCueCount; ++index) {
        const QString name = SoundEffects::cueName(static_cast<SoundEffects::Cue>(index));
        QFile file(QStringLiteral(":/sounds/%1.wav").arg(name));
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(name));
        const QByteArray header = file.read(44);
        QCOMPARE(header.size(), 44);
        QCOMPARE(header.mid(0, 4), QByteArray("RIFF"));
        QCOMPARE(header.mid(8, 8), QByteArray("WAVEfmt "));
        QCOMPARE(qFromLittleEndian<quint16>(header.constData() + 20), quint16(1));   // PCM
        QCOMPARE(qFromLittleEndian<quint16>(header.constData() + 22), quint16(1));   // mono
        QCOMPARE(qFromLittleEndian<quint16>(header.constData() + 34), quint16(16));  // bits per sample
        QVERIFY2(file.size() > 1000, qPrintable(name));
    }
#endif
}

void SoundEffectsTest::cues_loadOnTheAudioDevice() {
#ifndef TAPOCALYPSE_HAS_SOUND
    QSKIP("built without Qt Multimedia");
#else
    // CI runners have no sound card; there QSoundEffect reports an error by design, so only check real devices.
    if (QMediaDevices::defaultAudioOutput().isNull()) QSKIP("no audio output device");
    SoundEffects sounds(settingsPath());
    QVERIFY(sounds.available());
    for (auto* effect : sounds.m_effects) {
        QVERIFY(effect);
        QTRY_VERIFY2(effect->status() != QSoundEffect::Loading && effect->status() != QSoundEffect::Null, qPrintable(effect->source().toString()));
        QVERIFY2(effect->status() == QSoundEffect::Ready, qPrintable(effect->source().toString()));
    }
#endif
}

QTEST_GUILESS_MAIN(SoundEffectsTest)
#include "test_soundeffects.moc"
