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
namespace mixxx {
using Vamp::Plugin;
using Vamp::PluginHostAdapter;

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QStringBuilder>

VampPluginLoader::VampPluginLoader()
{
    m_pVampPluginLoader = PluginLoader::getInstance();
}

VampPluginLoader::PluginKey VampPluginLoader::composePluginKey(
    std::string libraryName, std::string identifier)
{
    QMutexLocker lock(&s_mutex);
    auto key = m_pVampPluginLoader->composePluginKey(libraryName, identifier);
    return key;
}

VampPluginLoader::PluginCategoryHierarchy VampPluginLoader::getPluginCategory(
    Vamp::HostExt::PluginLoader::PluginKey plugin)
{
    QMutexLocker lock(&s_mutex);
    return m_pVampPluginLoader->getPluginCategory(plugin);
}

VampPluginLoader::PluginKeyList VampPluginLoader::listPlugins()
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
}
