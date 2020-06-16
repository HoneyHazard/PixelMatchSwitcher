#pragma once

#include <QListWidget>

#include "pm-structs.hpp"

class PmCore;

class PmListWidget : public QListWidget
{
public:
    PmListWidget(PmCore* core, QWidget* parent);

signals:
    void sigNewMatchConfig(size_t idx, PmMatchConfig cfg);
    void sigNoMatchSceneChanged(std::string scene, std::string transition);
    void sigSelectMatchIndex(size_t matchIndex);

protected slots:
    void onNewMatchResults(size_t idx, PmMatchResults results);

    void onNewMultiMatchConfigSize(size_t sz);
    void onNewMatchConfig(size_t idx, PmMatchConfig cfg);
    void onNoMatchSceneChanged(std::string scene, std::string transition);
    void onSelectMatchIndex(size_t matchIndex, PmMatchConfig config);

};