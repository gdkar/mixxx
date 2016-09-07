#ifndef __RECORDING_DEFS_H__
#define __RECORDING_DEFS_H__

#define RECORDING_PREF_KEY "[Recording]"
#define ENCODING_WAVE "WAV"
#define ENCODING_AIFF "AIFF"
#define ENCODING_OGG  "OGG"
#define ENCODING_MP3 "MP3"

#define RECORD_OFF 0.0
#define RECORD_READY 1.0
#define RECORD_ON 2.0

//File options for preferences Splitting
#define SPLIT_650MB "650 MB (CD)"
#define SPLIT_700MB "700 MB (CD)"
#define SPLIT_1024MB "1 GB"
#define SPLIT_2048MB "2 GB"
#define SPLIT_4096MB "4 GB"

// Byte conversions Instead of multiplying megabytes with 1024 to get kilobytes
// I use 1000 Once the recording size has reached there's enough room to add
// closing frames by the encoder. All sizes are in bytes.
#define SIZE_650MB  Q_UINT64_C(650000000)
#define SIZE_700MB  Q_UINT64_C(750000000)
#define SIZE_1GB    Q_UINT64_C(1000000000)
#define SIZE_2GB    Q_UINT64_C(2000000000)
#define SIZE_4GB    Q_UINT64_C(4000000000)

#endif
