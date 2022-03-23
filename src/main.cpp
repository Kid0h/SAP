#include <iostream>
#include <string.h>
#include <thread>

#include <fmt/core.h>
#include <fmt/color.h>

extern "C" {
    #include <libavutil/avutil.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
    #include <libavutil/audio_fifo.h>
}

#ifdef SAP_DEBUG
#undef MA_DEBUG_OUTPUT
#endif

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#ifdef WIN32 // Enable console colors on Windows
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#define LOG_LOG fmt::print(fg(fmt::color::green),   "LOG: ")
#define LOG_INF fmt::print(fg(fmt::color::blue),    "INFO: ")
#define LOG_WAR fmt::print(fg(fmt::color::yellow),  "WARNING: ")
#define LOG_ERR fmt::print(fg(fmt::color::red),     "ERROR: ")

void audio_data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count);
void av_log_callback(void*, int level, const char* buffer, va_list vargs);

bool logging;

int main(int argc, char* argv[]) {
    char* file_path = nullptr;
    int volume    = -1;
    logging = false;

    #ifdef WIN32 // Enable console colors on Windows
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    #endif

    // Parse arguments
    if (argc < 2) { LOG_ERR; fmt::print("Please specifiy a file path\n"); return -1; }
    // Volume
    if (argc >= 3) {
        if (argv[2][0] != '-') {
            try { volume = atoi(argv[2]); }
            catch(...) { }
            if (volume < 0 || volume > 100) {
                LOG_ERR; fmt::print("Please enter a valid volume number (0 - 100)\n");
                return -1;
            }
        }
    }
    for (int i = 1; i < argc; ++i) {
        // Logging 
        if (strcmp(argv[i], "--log") == 0 || strcmp(argv[i], "-l") == 0) {
            logging = true;
        }
    } file_path = argv[1];

    int stream_index = -1;
    const AVCodec* codec = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVStream* stream = nullptr;

    // Set logging
    av_log_set_callback(av_log_callback);

    /* Open audio file */
    int status = -1;
    AVFormatContext* format_ctx = avformat_alloc_context();
    status = avformat_open_input(&format_ctx, file_path, NULL, NULL);
    if (status < 0) {
        LOG_ERR; fmt::print("Could not open \"{}\"\n", file_path);
        return -1;
    }

    // Find stream info
    status = avformat_find_stream_info(format_ctx, NULL);
    if (status < 0) {
        LOG_ERR; fmt::print("Could not find stream info\n");
        return -1;
    }

    // Find audio stream
    stream_index = av_find_best_stream(format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (stream_index < 0) {
        LOG_ERR; fmt::print("Could not find an audio stream inside \"{}\"\n", file_path);
        return -1;
    }
    stream = format_ctx->streams[stream_index];

    /* Decode audio */
    // Find a codec
    codec = avcodec_find_decoder(format_ctx->streams[stream_index]->codecpar->codec_id);
    if (!codec) {
        LOG_ERR; fmt::print("Could not find a valud audio codec for \"{}\"\n", file_path);
        return -1;
    }

    // Allocate codec context
    codec_ctx = avcodec_alloc_context3(codec);

    // Copy stream parameters to codec
    avcodec_parameters_to_context(codec_ctx, stream->codecpar);

    // Open codec
    status = avcodec_open2(codec_ctx, codec, nullptr);
    if (status < 0) {
        LOG_ERR; fmt::print("Could not open codec\n");
        return -1;
    }

    // Decode audio data
    AVPacket* packet    = av_packet_alloc();
    AVFrame* frame      = av_frame_alloc();

    SwrContext* swr_ctx = swr_alloc_set_opts(
        nullptr, stream->codecpar->channel_layout, AV_SAMPLE_FMT_FLT, stream->codecpar->channel_layout,
        stream->codecpar->channel_layout, (AVSampleFormat)stream->codecpar->format, stream->codecpar->sample_rate, 0, nullptr
    );

    AVAudioFifo* fifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLT, stream->codecpar->channels, 1);

    // Play
    fmt::print(fg(fmt::color::green), "Playing: "); fmt::print("\"{}\"", file_path); std::flush(std::cout);
    while (av_read_frame(format_ctx, packet) == 0) {
        if (packet->stream_index != stream_index) continue;

        status = avcodec_send_packet(codec_ctx, packet);
        if (status < 0) {
            if (status != AVERROR(EAGAIN)) {
                LOG_ERR; fmt::print("Uh oh.\n");
            }
        }

        while ((status = avcodec_receive_frame(codec_ctx, frame)) == 0) {
            // Resample frame
            AVFrame* resampled_frame = av_frame_alloc();
            resampled_frame->sample_rate    = frame->sample_rate;
            resampled_frame->channel_layout = frame->channel_layout;
            resampled_frame->channels       = frame->channels;
            resampled_frame->format         = AV_SAMPLE_FMT_FLT;

            status = swr_convert_frame(swr_ctx, resampled_frame, frame);
            av_frame_unref(frame);
            av_audio_fifo_write(fifo, (void**)resampled_frame->data, resampled_frame->nb_samples);
            av_frame_free(&resampled_frame);
        }
    }

    /* Play audio */
    ma_device device;
    ma_device_config device_config;

    // Initialize playback device config
    device_config = ma_device_config_init(ma_device_type_playback);
    device_config.playback.format   = ma_format_f32;
    device_config.playback.channels = stream->codecpar->channels;
    device_config.sampleRate        = stream->codecpar->sample_rate;
    device_config.dataCallback      = audio_data_callback;
    device_config.pUserData         = fifo;

    avformat_close_input(&format_ctx);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    swr_free(&swr_ctx);

    // Initialize playback device
    if (ma_device_init(nullptr, &device_config, &device) != MA_SUCCESS) {
        LOG_ERR; fmt::print("Could not initialize playback device\n");
        return -1;
    }

    // Set playback volume
    ma_device_set_master_volume(&device, volume * 0.01f);

    // Start playback device
    if (ma_device_start(&device) != MA_SUCCESS) {
        LOG_ERR; fmt::print("Could not start playback device\n");
        ma_device_uninit(&device);
        return -1;
    } 

    while (av_audio_fifo_size(fifo) != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    ma_device_uninit(&device);

    return 0;
}

void audio_data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count) {
    AVAudioFifo* fifo = reinterpret_cast<AVAudioFifo*>(device->pUserData);
    av_audio_fifo_read(fifo, &output, frame_count);
    (void)input;
}

void av_log_callback(void*, int level, const char* buffer, va_list vargs) {
    // Something went terribly wrong - crash
    if (level == AV_LOG_PANIC) {
        LOG_ERR; fmt::print(buffer);
        exit(-1);
    }

    // Something went wrong but not all future data is affected - continue
    else if (level == AV_LOG_ERROR) {
        LOG_ERR; fmt::print(buffer);
    }

    // Something may be wrong - continue
    else if (level == AV_LOG_WARNING) {
        if (logging) {
            LOG_WAR; fmt::print(buffer);
        }
    }

    // Standard information - continue
    else if (level == AV_LOG_INFO) {
        if (logging) {
            LOG_INF; fmt::print(buffer);
        }
    }
}
