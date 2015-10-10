// schemamanager.cpp
// Created 12/29/2009 by RJ Ryan (rryan@mit.edu)

#include <QtDebug>

#include "library/schemamanager.h"
#include "library/queryutil.h"
#include "util/xml.h"
#include "util/assert.h"

const QString SchemaManager::SETTINGS_VERSION_STRING = "mixxx.schema.version";
const QString SchemaManager::SETTINGS_MINCOMPATIBLE_STRING = "mixxx.schema.min_compatible_version";

// static
SchemaManager::Result SchemaManager::upgradeToSchemaVersion(
        const QString& schemaFilename,
        QSqlDatabase& db, const int targetVersion)
{
    SettingsDAO settings(db);
    auto currentVersion = getCurrentSchemaVersion(settings);
    DEBUG_ASSERT_AND_HANDLE(currentVersion >= 0) {
        return RESULT_UPGRADE_FAILED;
    }
    if (currentVersion == targetVersion) {
        qDebug() << "SchemaManager::upgradeToSchemaVersion already at version"
                 << targetVersion;
        return RESULT_OK;
    } else if (currentVersion < targetVersion) {
        qDebug() << "SchemaManager::upgradeToSchemaVersion upgrading"
                 << targetVersion-currentVersion << "versions to version"
                 << targetVersion;
    } else {
        qDebug() << "SchemaManager::upgradeToSchemaVersion already past target "
                "version. currentVersion:"
                 << currentVersion << "targetVersion:"
                 << targetVersion;

        if (isBackwardsCompatible(settings, currentVersion, targetVersion)) {
            qDebug() << "Current schema version is backwards-compatible with" << targetVersion;
            return RESULT_OK;
        } else {
            return RESULT_BACKWARDS_INCOMPATIBLE;
        }
    }
    qDebug() << "Loading schema" << schemaFilename;
    auto schemaRoot = XmlParse::openXMLFile(schemaFilename, "schema");
    if (schemaRoot.isNull())
    {
        // Error parsing xml file
        return RESULT_SCHEMA_ERROR;
    }
    auto revisions = schemaRoot.childNodes();
    QMap<int, QDomElement> revisionMap;
    for (auto i = 0; i < revisions.count(); i++) {
        auto revision = revisions.at(i).toElement();
        auto version = revision.attribute("version");
        DEBUG_ASSERT_AND_HANDLE(!version.isNull()) {
            // xml file is not valid
            return RESULT_SCHEMA_ERROR;
        }
        auto iVersion = version.toInt();
        revisionMap[iVersion] = revision;
    }
    // The checks above guarantee that currentVersion < targetVersion when we
    // get here.
    while (currentVersion < targetVersion)
    {
        auto thisTarget = currentVersion + 1;
        // Now that we bake the schema.xml into the binary it is a programming
        // error if we include a schema.xml that does not have information on
        // how to get all the way to targetVersion.
        if (!revisionMap.contains(thisTarget)) {
            qDebug() << "SchemaManager::upgradeToSchemaVersion"
                     << "Don't know how to get to"
                     << thisTarget << "from" << currentVersion;
            return RESULT_SCHEMA_ERROR;
        }
        auto revision = revisionMap[thisTarget];
        auto eDescription = revision.firstChildElement("description");
        auto eSql = revision.firstChildElement("sql");
        auto minCompatibleVersion = revision.attribute("min_compatible");
        // Default the min-compatible version to the current version string if
        // it's not in the schema.xml
        if (minCompatibleVersion.isNull())
        {
            minCompatibleVersion = QString::number(thisTarget);
        }
        DEBUG_ASSERT_AND_HANDLE(!eSql.isNull())
        {
            // xml file is not valid
            return RESULT_SCHEMA_ERROR;
        }
        auto description = eDescription.text();
        auto sql = eSql.text();
        qDebug() << "Applying version" << thisTarget << ":"
                 << description.trimmed();
        ScopedTransaction transaction(db);
        // TODO(XXX) We can't have semicolons in schema.xml for anything other
        // than statement separators.
        auto sqlStatements = sql.split(";");
        QStringListIterator it(sqlStatements);
        QSqlQuery query(db);
        auto result = true;
        while (result && it.hasNext())
        {
            auto statement = it.next().trimmed();
            if (statement.isEmpty()) continue;
            result = result && query.exec(statement);
            if (!result) {
                qDebug() << "Failed query:"
                         << statement
                         << query.lastError();
            }
        }
        if (result) {
            currentVersion = thisTarget;
            settings.setValue(SETTINGS_VERSION_STRING, thisTarget);
            settings.setValue(SETTINGS_MINCOMPATIBLE_STRING, minCompatibleVersion);
            transaction.commit();
        } else {
            qDebug() << "Failed to move from version" << currentVersion
                     << "to version" << thisTarget;
            transaction.rollback();
            return RESULT_UPGRADE_FAILED;
        }
    }
    return RESULT_OK;
}
// static
int SchemaManager::getCurrentSchemaVersion(SettingsDAO& settings)
{
    auto currentSchemaVersion = settings.getValue(SETTINGS_VERSION_STRING);
    // May be a null string if the schema has not been created. We default the
    // startVersion to 0 so that we automatically try to upgrade to revision 1.
    auto currentVersion = 0;
    if (!currentSchemaVersion.isNull()) currentVersion = currentSchemaVersion.toInt();
    return currentVersion;
}
// static
bool SchemaManager::isBackwardsCompatible(SettingsDAO& settings,int currentVersion,int targetVersion) {
    auto backwardsCompatibleVersion = settings.getValue(SETTINGS_MINCOMPATIBLE_STRING);
    auto  ok = false;
    auto iBackwardsCompatibleVersion = backwardsCompatibleVersion.toInt(&ok);

    // If the current backwards compatible schema version is not stored in the
    // settings table, assume the current schema version is only backwards
    // compatible with itself.
    if (backwardsCompatibleVersion.isNull() || !ok) {
        // rryan 11/2010 We just added the backwards compatible flags, and some
        // people using the Mixxx trunk are already on schema version 7. This
        // special case is for them. Schema version 7 is backwards compatible
        // with schema version 3.
        if (currentVersion == 7) {
            iBackwardsCompatibleVersion = 3;
        } else  iBackwardsCompatibleVersion = currentVersion;
    }
    // If the target version is greater than the minimum compatible version of
    // the current schema, then the current schema is backwards compatible with
    // targetVersion.
    return iBackwardsCompatibleVersion <= targetVersion;
}
