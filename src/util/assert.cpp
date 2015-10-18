#include "util/assert.h"

void mixxx_debug_assert(const char* assertion, const char* file, int line)
{
#ifdef MIXXX_DEBUG_ASSERTIONS_FATAL
    qFatal("DEBUG ASSERT: \"%s\" in file %s, line %d", assertion, file, line);
#else
    qWarning("DEBUG ASSERT: \"%s\" in file %s, line %d", assertion, file, line);
#endif
}
bool mixxx_maybe_debug_assert_return_true(const char* assertion, const char* file, int line)
{
#ifdef MIXXX_BUILD_DEBUG
    mixxx_debug_assert(assertion, file, line);
#else
    Q_UNUSED(assertion);
    Q_UNUSED(file);
    Q_UNUSED(line);
#endif
    return true;
}
void mixxx_release_assert(const char* assertion, const char* file, int line)
{
    qFatal("ASSERT: \"%s\" in file %s, line %d", assertion, file, line);
}
