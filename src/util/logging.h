#ifndef MIXXX_UTIL_LOGGING_H
#define MIXXX_UTIL_LOGGING_H

#include <QDir>

#include <QString>
#include <QMetaObject>
#include <QObject>
#include <QMetaEnum>

namespace mixxx {

enum class LogLevel {
    Critical = 0,
    Warning = 1,
    Info = 2,
    Debug = 3,
    Trace = 4, // for profiling etc.
    Default = Warning,
};

class Logging {
    Q_GADGET
  public:
    using LogLevel = mixxx::LogLevel;
    Q_ENUM(LogLevel);
    static constexpr LogLevel kLogLevelDefault = LogLevel::Warning;
    // Any debug statement starting with this prefix bypasses the --logLevel
    // command line flags.
    static constexpr const char* kControllerDebugPrefix = "CDBG";

    // These are not thread safe. Only call them on Mixxx startup and shutdown.
    static void initialize(const QDir& settingsDir,
                           LogLevel logLevel,
                           bool debugAssertBreak);
    static void shutdown();

    // Query the current log level
    static LogLevel logLevel() {
        return s_logLevel;
    }
    static bool enabled(LogLevel logLevel) {
        return s_logLevel >= logLevel;
    }
    static bool traceEnabled() {
        return enabled(LogLevel::Trace);
    }
    static bool debugEnabled() {
        return enabled(LogLevel::Debug);
    }
    static bool infoEnabled() {
        return enabled(LogLevel::Info);
    }

  private:
    Logging() = delete;

    static LogLevel s_logLevel;
};

}  // namespace mixxx

#endif /* MIXXX_UTIL_LOGGING_H */
