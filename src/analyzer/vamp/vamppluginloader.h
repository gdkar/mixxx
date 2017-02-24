/*
 * vamppluginloader.h
 *
 *  Created on: 23/jan/2012
 *      Author: Tobias Rafreider
 *
 * This is a thread-safe wrapper class around Vamp's
 * PluginLoader class.
 */

#ifndef ANALYZER_VAMP_VAMPPLUGINLOADER_H
#define ANALYZER_VAMP_VAMPPLUGINLOADER_H

#include <QMutex>
#include <vamp-hostsdk/vamp-hostsdk.h>
#include <string>

#include "util/class.h"

using Vamp::Plugin;
using Vamp::PluginHostAdapter;
using Vamp::HostExt::PluginLoader;
using Vamp::HostExt::PluginWrapper;
using Vamp::HostExt::PluginInputDomainAdapter;

class VampPluginLoader {
  private:
    VampPluginLoader();
    virtual ~VampPluginLoader();

  public:
    using Plugin = Vamp::Plugin;
    using PluginLoader = Vamp::HostExt::PluginLoader;
    using PluginKey = PluginLoader::PluginKey;
    using PluginKeyList = PluginLoader::PluginKeyList;
    using PluginCategoryHierarchy = PluginLoader::PluginCategoryHierarchy;

    static VampPluginLoader* getInstance();
    PluginKeyList listPlugins();
    Plugin *loadPlugin(PluginKey, float inputSampleRate, int adapterFlags = 0);
    PluginKey composePluginKey(std::string libraryName, std::string identifier);
    PluginCategoryHierarchy getPluginCategory(PluginKey plugin);

  private:
    static VampPluginLoader* s_instance;
    static QMutex s_mutex;
    PluginLoader* m_pVampPluginLoader;
    DISALLOW_COPY_AND_ASSIGN(VampPluginLoader);
};

#endif // ANALYZER_VAMP_VAMPPLUGINLOADER_H
