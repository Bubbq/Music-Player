#include "headers/stream_management.h"

int get_metadata_parameter(AVDictionary* stream_metadata, const char* parameter, int maxlen, char dst[maxlen])
{
    AVDictionaryEntry* tag = av_dict_get(stream_metadata, parameter, NULL, AV_DICT_IGNORE_SUFFIX);
    if(tag) {
        if(strlen(tag->value) < maxlen) {
            strcpy(dst, tag->value);
            return 0;
        }

        else 
            return -1;
    }

    else 
        return -1;
}

int extract_cover_image(const char* song_path, const char* output_jpeg_path)
{
    AVFormatContext* format_context = get_format_context(song_path);
    
    if(format_context == NULL) {
        avformat_close_input(&format_context);
        return -1;
    }

    // the attached picture occurs when the stream type is of 'AV_DISPOSITION_ATTACHED_PIC'
    for (unsigned int i = 0; i < format_context->nb_streams; i++) {

        AVStream *stream = format_context->streams[i];
        if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {

            AVPacket pkt = stream->attached_pic;
            FILE *f = fopen(output_jpeg_path, "wb");
            if (!f) {
                printf("error writing image data to \"%s\"\n", output_jpeg_path);
                avformat_close_input(&format_context);
                return -1;
            }

            fwrite(pkt.data, 1, pkt.size, f);
            fclose(f);
            avformat_close_input(&format_context);
            return 0;  
        }
    }

    // no picture extracted case
    printf("no image extracted from \"%s\"\n", format_context->url);
    avformat_close_input(&format_context);
    return -1;
}

AVFormatContext* get_format_context(const char *path)
{
    AVFormatContext *format_context = NULL;

    if (avformat_open_input(&format_context, path, NULL, NULL) < 0) {
        printf("error opening the file \"%s\"\n", path);
        return NULL;
    }

    if (avformat_find_stream_info(format_context, NULL) < 0) {
        printf("error finding stream info for the file \"%s\"\n", path);
        avformat_close_input(&format_context);
        return NULL;
    }

    return format_context;
}