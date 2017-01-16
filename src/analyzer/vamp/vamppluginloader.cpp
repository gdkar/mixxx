/*
 * vamppluginloader.cpp
 *
 *  Created on: 23/jan/2012
 *      Author: Tobias Rafreider
 *
 * This is a thread-safe wrapper class around Vamp's
 * PluginLoader class.
 */
#include "analyzer/vamp/vamppluginloader.h"

using Vamp::Plugin;
using Vamp::PluginHostAdapter;

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QStringBuilder>

VampPluginLoader::VampPluginLoader()
{
    m_pVampPluginLoader = Vamp::HostExt::PluginLoader::getInstance();
}

VampPluginLoader::~VampPluginLoader() = default;

VampPluginLoader* VampPluginLoader::getInstance()
{
    QMutexLocker lock(&s_mutex);
    if (!s_instance)
        s_instance = new VampPluginLoader();
    return s_instance;
}

PluginLoader::PluginKey VampPluginLoader::composePluginKey(
    std::string libraryName, std::string identifier)
{
    QMutexLocker lock(&s_mutex);
    auto key = m_pVampPluginLoader->composePluginKey(libraryName, identifier);
    return key;
}

PluginLoader::PluginCategoryHierarchy VampPluginLoader::getPluginCategory(
    Vamp::HostExt::PluginLoader::PluginKey plugin)
{
    QMutexLocker lock(&s_mutex);
    return m_pVampPluginLoader->getPluginCategory(plugin);
}

PluginLoader::PluginKeyList VampPluginLoader::listPlugins()
{
    QMutexLocker lock(&s_mutex);
    return m_pVampPluginLoader->listPlugins();
}

Vamp::Plugin* VampPluginLoader::loadPlugin(
    Vamp::HostExt::PluginLoader::PluginKey key,
    float inputSampleRate, int adapterFlags)
{
    QMutexLocker lock(&s_mutex);
    return m_pVampPluginLoader->loadPlugin(key, inputSampleRate, adapterFlags);
}
