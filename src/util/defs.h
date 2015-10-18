_Pragma("once")
#include <cstddef>
// Used for returning errors from functions.
typedef bool Result;
const Result OK  = true;
const Result ERR = false;
// Maximum buffer length to each EngineObject::process call.
const size_t MAX_BUFFER_LEN = (1ul <<18);

