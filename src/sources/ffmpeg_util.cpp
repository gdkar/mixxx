#include "sources/ffmpeg_util.h"

QString ff_make_error_string(int err)
{
  char str[64];
  av_strerror(err,str,sizeof(str));
  return QString{str};
}
