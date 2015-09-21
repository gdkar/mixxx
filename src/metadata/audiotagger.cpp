#include "metadata/audiotagger.h"
#include "metadata/trackmetadatataglib.h"
namespace {
SecurityTokenPointer openSecurityToken(QFileInfo file,
        const SecurityTokenPointer& pToken) {
    if (pToken.isNull()) {return Sandbox::openSecurityToken(file, true);}
    else {return pToken;}
}
} // anonymous namespace
AudioTagger::AudioTagger(const QString& file, SecurityTokenPointer pToken) :
        m_file(file),
        m_pSecurityToken(openSecurityToken(m_file, pToken)) {
}
bool AudioTagger::save(const Mixxx::TrackMetadata& trackMetadata) {
    return writeTrackMetadataIntoFile(trackMetadata, m_file.canonicalFilePath());
}
