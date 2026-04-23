#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class GameLogicBridge : public QObject {
    Q_OBJECT

   public:
    explicit GameLogicBridge(QObject* parent = nullptr);

    Q_INVOKABLE QVariantMap analyzeSelection(const QVariantList& rawGrid, int row, int col, int minimumGroupSize) const;
    Q_INVOKABLE QVariantMap applyMove(const QVariantList& rawGrid, int row, int col, int minimumGroupSize) const;
    Q_INVOKABLE QVariantMap planMove(const QVariantList& rawGrid, int row, int col, int minimumGroupSize) const;
    Q_INVOKABLE bool hasMoves(const QVariantList& rawGrid, int minimumGroupSize) const;
};
