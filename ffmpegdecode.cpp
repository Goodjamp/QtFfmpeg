#include <QDebug>

#include "ffmpegdecode.h"

#define INBUF_SIZE 4096

FFmpegDecode::FFmpegDecode(QObject *parent) : QObject(parent)
{

    qDebug()<<"Video version"<<avutil_version();
    qDebug()<<"Video version"<<avutil_license();
    qDebug()<<"Video version"<<av_version_info();

   //const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    //qDebug()<<"Codec name"<<codec->long_name;
}

const char *FFmpegDecode::getffmpegInfo()
{
    return av_version_info();
}

FFmpegDecode::FFmpegStatus FFmpegDecode::closeDecoder()
{
    file.close();

    return FFmpegDecode::FFMPEG_OK;
}

/*
 * CB function to set the desirable output frame format
 * Let's it will be YUV420
 */
enum AVPixelFormat getFormat(struct AVCodecContext *s, const enum AVPixelFormat *fmt)
{
    if (*fmt == AV_PIX_FMT_NONE) {
        return AV_PIX_FMT_NONE;
    }
    do {
        if (AV_PIX_FMT_YUV420P == *fmt) {
            qDebug()<<"Propese format = "<<*fmt;
            //return AV_PIX_FMT_YUV420P;
        }
        qDebug()<<"Propese format = "<<*fmt;
    } while(*(++fmt) != AV_PIX_FMT_NONE);

    return AV_PIX_FMT_NONE;
}

/*
 * The metho is:
 * - capture the video stream from the Web camera
 * - decode it to the frame
 * - - show freame on the screen
 * - encode frame to the HD264 stream
 * - save stream to the output file
 */
FFmpegDecode::FFmpegStatus FFmpegDecode::saveCameraStream(QString outFileName)
{
    QFile outFile(outFileName);
    outFile.open(QIODevice::ReadWrite);

    avdevice_register_all();

    /*
     * To encode the camera stream we need to now the pixel format that will be return by the decoder. So that, first we need to find decoder for the camera stream
     */

    inputFormat = av_find_input_format("v4l2"); // video for linux (v4l2) input camera format
    av_dict_set(&dictionaryOptions, "framerate", "30", 0);
    av_dict_set(&dictionaryOptions, "video_size", "320x240",  0);

    if (avformat_open_input(&formatContext, cameraPath, inputFormat, &dictionaryOptions) != 0 ) {
        qDebug()<<"Can't connect camera";
        return FFmpegDecode::FFMPEG_OPEN_INPUT_PATH_STREAM_ERROR;
    } else {
        qDebug()<<formatContext->iformat->long_name;
    }

    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        qDebug()<<"Can't find stream";
        return FFmpegDecode::FFMPEG_FIND_INPUT_STREAM_ERROR;
    }

    qDebug()<<"Strems number = "<<formatContext->nb_streams;

    videoStreamInd = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

    /*
     * Find and alocate decoder for the camera stream
     */
    if (videoStreamInd >= 0) {
        codecDecodeParameters = formatContext->streams[videoStreamInd]->codecpar;
        codecDecode = avcodec_find_decoder(codecDecodeParameters->codec_id);
        qDebug()<<"Codec name "<<codecDecode->long_name;
        if (codecDecode->pix_fmts != NULL) {
            const AVPixelFormat *pxFormat = codecDecode->pix_fmts;
            do {
                qDebug()<<"Pixel format"<<*pxFormat;
            } while (*(++pxFormat) != AV_PIX_FMT_NONE);
        } else {
            qDebug()<<"Pixel format == AV_PIX_FMT_NONE";
        }
        qDebug()<<"Codec frame size "<<codecDecodeParameters->frame_size;
        qDebug()<<"Codec frame width "<<codecDecodeParameters->width;
        qDebug()<<"Codec frame height "<<codecDecodeParameters->height;

        codecDecodeContext = avcodec_alloc_context3(codecDecode);
        if (codecDecodeContext == NULL) {
            qDebug()<<"Can't allocate context 3";
            return FFmpegDecode::FFMPEG_ALLOCATE_CONTEXT3_ERROR;
        }
        if (avcodec_parameters_to_context(codecDecodeContext, codecDecodeParameters) < 0) {
            qDebug()<<"Parameter to context error";
            return FFmpegDecode::FFMPEG_PARAMETRS_TO_CONTEXT_ERROR;
        }
        codecDecodeContext->get_format = getFormat;

        if (avcodec_open2(codecDecodeContext, codecDecode, NULL) < 0) {
            qDebug()<<"Open codec error";
            return FFmpegDecode::FFMPEG_OPEN2_ERROR;
        }
    } else {
        return FFmpegDecode::FFMPEG_FIND_VIDE_STREAM_ERROR;
    }

    /*
     * Create and configure encoder
     */
    codecEncode= avcodec_find_decoder(AV_CODEC_ID_H264); // Find the codec by ID. Also we can find codec by the name with function avcodec_find_decoder_by_name(), for example "H264-MPEG-4"
    if (codecEncode == 0) {
        qDebug()<<"Can't find H264 encoder";
        return FFmpegDecode::FFMPEG_FIND_CODEC_DECODER_ERROR;
    }

    codecEncodeContext = avcodec_alloc_context3(codecEncode);
    if (codecEncodeContext == NULL) {
        qDebug()<<"Can't alock encode H264";
        return FFmpegDecode::FFMPEG_ALOCK_CODEC_DECODER_ERROR;
    }

    /* put sample parameters */
    codecEncodeContext->bit_rate = 400000;
    /* resolution must be a multiple of two */
    codecEncodeContext->width = 320;
    codecEncodeContext->height = 240;
    /* frames per second */
    codecEncodeContext->time_base = (AVRational){1, 25};
    codecEncodeContext->framerate = (AVRational){25, 1};

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    codecEncodeContext->gop_size = 10;
    codecEncodeContext->max_b_frames = 1;

    /*
     * pix_fmt - it is a format of frame pased to encoder. We need to take this information from the source of video data (camera)
     */
    qDebug()<<"Pix format sw: "<<codecDecodeContext->sw_pix_fmt;
    qDebug()<<"Pix format hw: "<<codecDecodeContext->pix_fmt;
    codecEncodeContext->pix_fmt = codecDecodeContext->pix_fmt;

    /*
     * Initilise the context with settings and oprions fron pass to the last arg.
     *
     */
    if (avcodec_open2(codecEncodeContext, codecEncode, NULL) < 0) {
        qDebug()<<"Open codec error";
        return FFmpegDecode::FFMPEG_OPEN_CODEC_DECODER_ERROR;
    }

    /*
     *   Init reading containers
     */
    pkt = av_packet_alloc();
    if (pkt == NULL) {
        qDebug()<<"Can't allk paket";
        return FFmpegDecode::FFMPEG_ALLOCATE_PKT_ERROR;
    }

    frame = av_frame_alloc();
    if (pkt == NULL) {
        qDebug()<<"Can't allk paket";
        return FFmpegDecode::FFMPEG_ALLOCATE_FRAME_ERROR;
    }

    return FFmpegDecode::FFMPEG_OK;
}

FFmpegDecode::FFmpegStatus FFmpegDecode::connectToFile()
{
    avdevice_register_all();

    /*
     * Open source of input data
     */
    if (avformat_open_input(&formatContext, filePath, NULL, NULL) != 0 ) {
        qDebug()<<"Can't connect file";
        return FFmpegDecode::FFMPEG_OPEN_FILE_ERROR;
    } else {
        qDebug()<<formatContext->iformat->long_name;
    }

    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        qDebug()<<"Can't find stream";
        return FFmpegDecode::FFMPEG_FIND_STREAM_ERROR;
    }

    qDebug()<<"Strems number = "<<formatContext->nb_streams;

    /*
     * The video container could contain multiple multimedia stream.
     * Take a index of video stream and found a suitable decoder
     */
    videoStreamInd = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (videoStreamInd >= 0) {
        codecDecodeParameters = formatContext->streams[videoStreamInd]->codecpar;
        codecDecode = avcodec_find_decoder(codecDecodeParameters->codec_id);

        qDebug()<<"Codec name "<<codecDecode->long_name;
        if (codecDecode->pix_fmts != NULL) {
            const AVPixelFormat *pxFormat = codecDecode->pix_fmts;
            do {
                qDebug()<<"Pixel format"<<*pxFormat;
            } while (*(++pxFormat) != AV_PIX_FMT_NONE);
        } else {
            qDebug()<<"Pixel format == AV_PIX_FMT_NONE";
        }
        qDebug()<<"Codec frame size "<<codecDecodeParameters->frame_size;
        qDebug()<<"Codec frame width "<<codecDecodeParameters->width;
        qDebug()<<"Codec frame height "<<codecDecodeParameters->height;

        /*
         *  Init codec
         */
        codecDecodeContext = avcodec_alloc_context3(codecDecode);
        if (codecDecodeContext == NULL) {
            qDebug()<<"Can't allocate context 3";
            return FFmpegDecode::FFMPEG_ALLOCATE_CONTEXT3_ERROR;
        }
        if (avcodec_parameters_to_context(codecDecodeContext, codecDecodeParameters) < 0) {
            qDebug()<<"Parameter to context error";
            return FFmpegDecode::FFMPEG_PARAMETRS_TO_CONTEXT_ERROR;
        }
        //pCodecContext->get_format = getFormat;
        if (avcodec_open2(codecDecodeContext, codecDecode, NULL) < 0) {
            qDebug()<<"Open codec error";
            return FFmpegDecode::FFMPEG_OPEN2_ERROR;
        }
    }

    /*
     *   Init reading containers
     */
    pkt = av_packet_alloc();
    if (pkt == NULL) {
        qDebug()<<"Can't allk paket";
        return FFmpegDecode::FFMPEG_ALLOCATE_PKT_ERROR;
    }

    frame = av_frame_alloc();
    if (pkt == NULL) {
        qDebug()<<"Can't allk paket";
        return FFmpegDecode::FFMPEG_ALLOCATE_FRAME_ERROR;
    }

    qDebug()<<"Read frame Ok";
    return FFmpegDecode::FFMPEG_OK;
}

FFmpegDecode::FFmpegStatus FFmpegDecode::connectCamerra()
{
    avdevice_register_all();
    inputFormat = av_find_input_format("v4l2");
    av_dict_set(&dictionaryOptions, "framerate", "30", 0);
    av_dict_set(&dictionaryOptions, "video_size", "320x240",  0);

    if (avformat_open_input(&formatContext, cameraPath, inputFormat, &dictionaryOptions) != 0 ) {
        qDebug()<<"Can't connect camera";
        return FFmpegDecode::FFMPEG_OPEN_FILE_ERROR;
    } else {
        qDebug()<<formatContext->iformat->long_name;
    }

    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        qDebug()<<"Can't find stream";
        return FFmpegDecode::FFMPEG_FIND_STREAM_ERROR;
    }

    qDebug()<<"Strems number = "<<formatContext->nb_streams;

    videoStreamInd = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (videoStreamInd >= 0) {
        codecDecodeParameters = formatContext->streams[videoStreamInd]->codecpar;
        codecDecode = avcodec_find_decoder(codecDecodeParameters->codec_id);
        qDebug()<<"Codec name "<<codecDecode->long_name;
        if (codecDecode->pix_fmts != NULL) {
            const AVPixelFormat *pxFormat = codecDecode->pix_fmts;
            do {
                qDebug()<<"Pixel format"<<*pxFormat;
            } while (*(++pxFormat) != AV_PIX_FMT_NONE);
        } else {
            qDebug()<<"Pixel format == AV_PIX_FMT_NONE";
        }
        qDebug()<<"Codec frame size "<<codecDecodeParameters->frame_size;
        qDebug()<<"Codec frame width "<<codecDecodeParameters->width;
        qDebug()<<"Codec frame height "<<codecDecodeParameters->height;
    }

    codecDecodeContext = avcodec_alloc_context3(codecDecode);
    if (codecDecodeContext == NULL) {
        qDebug()<<"Can't allocate context 3";
        return FFmpegDecode::FFMPEG_ALLOCATE_CONTEXT3_ERROR;
    }
    if (avcodec_parameters_to_context(codecDecodeContext, codecDecodeParameters) < 0) {
        qDebug()<<"Parameter to context error";
        return FFmpegDecode::FFMPEG_PARAMETRS_TO_CONTEXT_ERROR;
    }
    codecDecodeContext->get_format = getFormat;
    if (avcodec_open2(codecDecodeContext, codecDecode, NULL) < 0) {
        qDebug()<<"Open codec error";
        return FFmpegDecode::FFMPEG_OPEN2_ERROR;
    }

    pkt = av_packet_alloc();
    if (pkt == NULL) {
        qDebug()<<"Can't alloc paket";
        return FFmpegDecode::FFMPEG_ALLOCATE_PKT_ERROR;
    }

    frame = av_frame_alloc();
    if (frame == NULL) {
        qDebug()<<"Can't alloc frame";
        return FFmpegDecode::FFMPEG_ALLOCATE_FRAME_ERROR;
    }

    return FFmpegDecode::FFMPEG_OK;
}


QSize FFmpegDecode::getFrameSize()
{
    if (formatContext == NULL) {
        return QSize(0,0);
    }
    return QSize(formatContext->streams[videoStreamInd]->codecpar->width,
                 formatContext->streams[videoStreamInd]->codecpar->height);
}

FFmpegDecode::FFmpegStatus FFmpegDecode::readFrame(uint8_t *dstFrame)
{
    int ret = 0;
    while(av_read_frame(formatContext, pkt) >= 0) { // read stream

        /*
         * Decode only video stream
         */
        if (pkt->stream_index == videoStreamInd) {
            ret  = avcodec_send_packet(codecDecodeContext, pkt); // send (pass) encodet data to the codec driver
            if (ret < 0) {
                fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(ret));
                return FFmpegDecode::FFMPEG_OK;
            }
            if (avcodec_receive_frame(codecDecodeContext, frame) == 0) {
                //qDebug()<<"Frame Format"<<frame->format;
                //qDebug()<<"Frame Height"<<frame->height;
                //qDebug()<<"Frame Width"<<frame->width;
                //qDebug()<<"Frame Width"<<frame->buf[0]->size;
                //qDebug()<<"Frame Type (mpeg 4)"<<frame->pict_type;

                if (frame->format == 13 || frame->format == 12) {
                    /*
                     * Take a luminos data. On the YUV420
                     */
                    int linIndex = 0;
                    for (uint32_t k = 0; k < frame->width * frame->height; k += 1) {
                        if (k == linIndex * frame->width + linIndex) {
                            linIndex++;
                            *dstFrame++ = 255;
                        } else {
                           *dstFrame++ = frame->buf[0]->data[k];
                        }
                    }
                } else {
                    for (uint32_t k = 0; k < frame->width * frame->height * 2; k += 2) {
                        *dstFrame++ = frame->buf[0]->data[k];
                    }
                }

                break;

            }
        }
    }
    return FFmpegDecode::FFMPEG_OK;
}
