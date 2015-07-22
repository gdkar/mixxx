_Pragma("once")
// Used for returning errors from functions.
enum Result {
    OK = 0,
    ERR = -1
};

// Maximum buffer length to each EngineObject::process call.
const unsigned int MAX_BUFFER_LEN = 160000;
