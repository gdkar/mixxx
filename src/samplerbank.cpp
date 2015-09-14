#include <QFileDialog>
#include <QMessageBox>

#include "player.h"
#include "samplerbank.h"
#include "trackinfoobject.h"
#include "controlpushbutton.h"
#include "playermanager.h"
#include "util/assert.h"

SamplerBank::SamplerBank(PlayerManager* pPlayerManager)
        : QObject(pPlayerManager),
          m_pPlayerManager(pPlayerManager) {
    DEBUG_ASSERT(m_pPlayerManager);
    m_pLoadControl = new ControlPushButton(ConfigKey("[Sampler]", "LoadSamplerBank"));
    connect(m_pLoadControl, SIGNAL(valueChanged(double)),this, SLOT(slotLoadSamplerBank(double)));
    m_pLoadControl->setParent(this);
    m_pSaveControl = new ControlPushButton(ConfigKey("[Sampler]", "SaveSamplerBank"));
    connect(m_pSaveControl, SIGNAL(valueChanged(double)),this, SLOT(slotSaveSamplerBank(double)));
    m_pSaveControl->setParent(this);
}
SamplerBank::~SamplerBank() = default;
void SamplerBank::slotSaveSamplerBank(double v) {
    if (v == 0.0 || m_pPlayerManager == nullptr) {return;}
    auto filefilter = tr("Mixxx Sampler Banks (*.xml)");
    auto samplerBankPath = QFileDialog::getSaveFileName(
            nullptr, tr("Save Sampler Bank"),
            QString(),
            tr("Mixxx Sampler Banks (*.xml)"),
            &filefilter);
    if (samplerBankPath.isNull() || samplerBankPath.isEmpty()) {return;}
    // Manually add extension due to bug in QFileDialog
    // via https://bugreports.qt-project.org/browse/QTBUG-27186
    // Can be removed after switch to Qt5
    QFileInfo fileName(samplerBankPath);
    if (fileName.suffix().isEmpty()) {
        auto ext = filefilter.section(".",1,1);
        ext.chop(1);
        samplerBankPath.append(".").append(ext);
    }
    // The user has picked a new directory via a file dialog. This means the
    // system sandboxer (if we are sandboxed) has granted us permission to this
    // folder. We don't need access to this file on a regular basis so we do not
    // register a security bookmark.

    QFile file(samplerBankPath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(nullptr,
                             tr("Error Saving Sampler Bank"),
                             tr("Could not write the sampler bank to '%1'.")
                             .arg(samplerBankPath));
        return;
    }
    QDomDocument doc("SamplerBank");
    auto root = doc.createElement("samplerbank");
    doc.appendChild(root);
    for (unsigned int i = 0; i < m_pPlayerManager->numSamplers(); ++i) {
        auto pSampler = m_pPlayerManager->getSampler(i + 1);
        if (pSampler == nullptr) {
            continue;
        }
        auto samplerNode = doc.createElement(QString("sampler"));
        samplerNode.setAttribute("group", pSampler->getGroup());

        auto pTrack = pSampler->getLoadedTrack();
        if (pTrack) {
            auto samplerLocation = pTrack->getLocation();
            samplerNode.setAttribute("location", samplerLocation);
        }
        root.appendChild(samplerNode);
    }
    auto docStr = doc.toString();
    file.write(docStr.toUtf8().constData());
    file.close();
}
void SamplerBank::slotLoadSamplerBank(double v) {
    if (v == 0.0 || m_pPlayerManager == nullptr) {return;}

    auto samplerBankPath = QFileDialog::getOpenFileName(
            nullptr,
            tr("Load Sampler Bank"),
            QString(),
            tr("Mixxx Sampler Banks (*.xml)"));
    if (samplerBankPath.isEmpty()) {return;}
    // The user has picked a new directory via a file dialog. This means the
    // system sandboxer (if we are sandboxed) has granted us permission to this
    // folder. We don't need access to this file on a regular basis so we do not
    // register a security bookmark.
    QFile file(samplerBankPath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(nullptr,
                             tr("Error Reading Sampler Bank"),
                             tr("Could not open the sampler bank file '%1'.")
                             .arg(samplerBankPath));
        return;
    }

    QDomDocument doc;
    if (!doc.setContent(file.readAll())) {
        QMessageBox::warning(nullptr,
                             tr("Error Reading Sampler Bank"),
                             tr("Could not read the sampler bank file '%1'.")
                             .arg(samplerBankPath));
        return;
    }
    auto root = doc.documentElement();
    if(root.tagName() != "samplerbank") {
        QMessageBox::warning(nullptr,
                             tr("Error Reading Sampler Bank"),
                             tr("Could not read the sampler bank file '%1'.")
                             .arg(samplerBankPath));
        return;
    }
    auto n = root.firstChild();
    while (!n.isNull()) {
        auto e = n.toElement();
        if (!e.isNull()) {
            if (e.tagName() == "sampler") {
                auto group = e.attribute("group", "");
                auto location = e.attribute("location", "");
                if (!group.isEmpty()) {
                    if (location.isEmpty()) {
                        m_pPlayerManager->slotLoadTrackToPlayer(TrackPointer(), group);
                    } else {
                        m_pPlayerManager->slotLoadToPlayer(location, group);
                    }
                }

            }
        }
        n = n.nextSibling();
    }
    file.close();
}
