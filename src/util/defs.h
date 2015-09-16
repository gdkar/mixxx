_Pragma("once")
// Used for returning errors from functions.
typedef bool Result;
const Result OK  = true;
const Result ERR = false;
// Maximum buffer length to each EngineObject::process call.
const unsigned int MAX_BUFFER_LEN = 160000;

