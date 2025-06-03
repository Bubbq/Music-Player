#include <libavutil/dict.h>
#include <libavformat/avformat.h>

// write a parameter of a stream's metadata to dst
int get_metadata_parameter(AVDictionary* stream_metadata, const char* parameter, int maxlen, char dst[maxlen]);

// Writes the cover image of file path to dst
int extract_cover_image(const char* song_path, const char* output_jpeg_path);

// returns the context in an audio or video file
AVFormatContext* get_format_context(const char *path);