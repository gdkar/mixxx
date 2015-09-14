_Pragma("once")
extern "C" {
  #include <libavutil/avutil.h>
  #include <libavformat/avformat.h>
  #include <libavcodec/avcodec.h>
  #include <libavdevice/avdevice.h>
  #include <libswresample/swresample.h>
};
#include <QString>

QString ff_make_error_string(int err);
