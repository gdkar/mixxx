#ifndef DEFS_H
#define DEFS_H
#include <qglobal.h>
#include <qmetatype.h>
#include <qobject.h>
// Used for returning errors from functions.
enum Result {
    OK = 0,
    ERR = -1
};
Q_DECLARE_METATYPE(Result);
Q_DECLARE_TYPEINFO(Result,Q_PRIMITIVE_TYPE);
// Maximum buffer length to each EngineObject::process call.
const unsigned int MAX_BUFFER_LEN = 160000;

#endif /* DEFS_H */
