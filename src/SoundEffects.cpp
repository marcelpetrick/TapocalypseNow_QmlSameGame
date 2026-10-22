#include "SoundEffects.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#ifdef TAPOCALYPSE_HAS_SOUND
#include <QSoundEffect>
#endif

namespace {

constexpr auto kMutedKey = "sound/muted";
// Cues overlap often (pop, then land); keep them below full scale so stacked cues do not clip.
constexpr float kVolume = 0.7F;

}  // namespace

SoundEffects::SoundEffects(const QString& settingsPath, QObject* parent) : QObject(parent), m_settingsPath(settingsPath) {
    const QSettings settings(settingsFile(), QSettings::IniFormat);
    // --benchmark plays dozens of moves per second; a burst of pops would only get in the way of profiling.
    const QStringList arguments = QCoreApplication::arguments();
    const bool sessionMuted = arguments.contains(QStringLiteral("--mute")) || arguments.contains(QStringLiteral("--benchmark"));
    m_muted = settings.value(QLatin1String(kMutedKey), false).toBool() || sessionMuted;

#ifdef TAPOCALYPSE_HAS_SOUND
    // Load every cue up front: QSoundEffect decodes asynchronously, and a cue that is still loading stays silent.
    for (int index = 0; index < kCueCount; ++index) {
        auto* effect = new QSoundEffect(this);
        effect->setSource(QUrl(QStringLiteral("qrc:/sounds/%1.wav").arg(cueName(static_cast<Cue>(index)))));
        effect->setVolume(kVolume);
        m_effects[index] = effect;
    }
#endif
}

SoundEffects::~SoundEffects() = default;

bool SoundEffects::available() const {
#ifdef TAPOCALYPSE_HAS_SOUND
    return true;
#else
    return false;
#endif
}

bool SoundEffects::muted() const { return m_muted; }

void SoundEffects::setMuted(bool muted) {
    if (m_muted == muted) return;
    m_muted = muted;
    QSettings settings(settingsFile(), QSettings::IniFormat);
    settings.setValue(QLatin1String(kMutedKey), muted);
    emit mutedChanged();
}

void SoundEffects::setVolume(float volume) {
#ifdef TAPOCALYPSE_HAS_SOUND
    for (auto* effect : m_effects) effect->setVolume(volume);
#else
    Q_UNUSED(volume);
#endif
}

SoundEffects::Cue SoundEffects::popCue(int groupSize) {
    if (groupSize >= 12) return Cue::Pop4;
    if (groupSize >= 8) return Cue::Pop3;
    if (groupSize >= 5) return Cue::Pop2;
    return Cue::Pop1;
}

QString SoundEffects::cueName(Cue cue) {
    switch (cue) {
        case Cue::Pop1:
            return QStringLiteral("pop1");
        case Cue::Pop2:
            return QStringLiteral("pop2");
        case Cue::Pop3:
            return QStringLiteral("pop3");
        case Cue::Pop4:
            return QStringLiteral("pop4");
        case Cue::Reject:
            return QStringLiteral("reject");
        case Cue::Land:
            return QStringLiteral("land");
        case Cue::Deal:
            return QStringLiteral("deal");
        case Cue::GameOver:
            return QStringLiteral("gameover");
        case Cue::Highscore:
            return QStringLiteral("highscore");
    }
    return {};
}

void SoundEffects::playPop(int groupSize) { play(popCue(groupSize)); }

void SoundEffects::playReject() { play(Cue::Reject); }

void SoundEffects::playLand() { play(Cue::Land); }

void SoundEffects::playDeal() { play(Cue::Deal); }

void SoundEffects::playGameOver() { play(Cue::GameOver); }

void SoundEffects::playHighscore() { play(Cue::Highscore); }

void SoundEffects::play(Cue cue) {
    if (m_muted) return;
#ifdef TAPOCALYPSE_HAS_SOUND
    m_effects[static_cast<int>(cue)]->play();
#endif
    emit cuePlayed(cueName(cue));
}

QString SoundEffects::settingsFile() const {
    if (!m_settingsPath.isEmpty()) return m_settingsPath;
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).filePath(QStringLiteral("settings.ini"));
}
