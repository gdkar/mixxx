#ifndef MIXXX_VAMPPLUGINLOADER_H
#define MIXXX_VAMPPLUGINLOADER_H

#include <vamp-hostsdk/vamp-hostsdk.h>
#include <memory>
#include <string>
#include <utility>
#include <QMutex>
namespace mixxx {

class VampPluginLoader final {
  public:
    VampPluginLoader();
    VampPluginLoader(VampPluginLoader && other) noexcept = default;

  public:
    using Plugin = Vamp::Plugin;
    using PluginLoader = Vamp::HostExt::PluginLoader;
    using PluginKey = PluginLoader::PluginKey;
    using PluginKeyList = PluginLoader::PluginKeyList;
    using PluginCategoryHierarchy = PluginLoader::PluginCategoryHierarchy;

    PluginKeyList listPlugins();
    Plugin *loadPlugin(PluginKey, float inputSampleRate, int adapterFlags = 0);
    PluginKey composePluginKey(std::string libraryName, std::string identifier);
    PluginCategoryHierarchy getPluginCategory(PluginKey plugin);

  private:
    static QMutex s_mutex;
    PluginLoader* m_pVampPluginLoader;
};

} // namespace mixxx


#endif // MIXXX_VAMPPLUGINLOADER_H
