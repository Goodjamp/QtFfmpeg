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

FFmpegDecode::FFmpegStatus FFmpegDecode::initDecoder(QString fileName)
{
    pkt = av_packet_alloc();
    if (!pkt) {
        return FFMPEG_ALLOC_AV_ERROR;
    }

    /* find the MPEG-1 video decoder */
    codec = avcodec_find_decoder(AV_CODEC_ID_MPEG1VIDEO);
    if (!codec) {
        return FFMPEG_FIND_DECODER_ERROR;
    }

    /*
     * Init parcer.
     * Parcer is used to parse the input video frame by frame.\
     * Explanation: sometimes we can't read the input (video) file,\
     * because the size of the file is too big. To trick this limitation,
     * we read a small piece of data and push it to the parser. After
     * completing reading the next frame, the parser is notified that the
     * new code frame is ready and we can go to the next decoding step.
     */
    parser = av_parser_init(codec->id);
    if (!parser) {
        return FFMPEG_PARSER_INIT_ERROR;
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        return FFMPEG_ALLOC_CON3_ERROR;
    }

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */

    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        return FFMPEG_OPEN2_ERROR;
    }

    file.setFileName(fileName);
    if (file.open(QIODevice::ReadOnly) == false) {
        return FFMPEG_OPEN_FILE_ERROR;
    }

    frame = av_frame_alloc();
    if (!frame) {
        return FFMPEG_ALLOC_FRAME_ERROR;
    }

    return FFMPEG_OK;
}

FFmpegDecode::FFmpegStatus FFmpegDecode::closeDecoder()
{
    file.close();

    return FFmpegDecode::FFMPEG_OK;
}

enum AVPixelFormat getFormat(struct AVCodecContext *s, const enum AVPixelFormat * fmt)
{
    const enum AVPixelFormat *fmtLocal = fmt;

    do {
       qDebug()<<"Propese format = "<<*fmtLocal;
    } while(*(++fmtLocal) != AV_PIX_FMT_NONE);
    qDebug()<<"*fmtLocal";
}

FFmpegDecode::FFmpegStatus FFmpegDecode::connectCamerra()
{
    avdevice_register_all();
    inputFormat = av_find_input_format("v4l2");
    av_dict_set(&options, "framerate", "25", 0);

    if (avformat_open_input(&pAVFormatContext, cameraPath, inputFormat, &options) != 0 ) {
        qDebug()<<"Can't connect camera";
        return FFmpegDecode::FFMPEG_OPEN_FILE_ERROR;
    } else {
        qDebug()<<pAVFormatContext->iformat->long_name;
    }

    if (avformat_find_stream_info(pAVFormatContext, NULL) < 0) {
        qDebug()<<"Can't find stream";
        return FFmpegDecode::FFMPEG_FIND_STREAM_ERROR;
    }

    qDebug()<<"Strems number = "<<pAVFormatContext->nb_streams;

    for (uint32_t k = 0; k < pAVFormatContext->nb_streams; k++) {
        pCodecParameters = pAVFormatContext->streams[k]->codecpar;

        if (pCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
                    pLocalCodec = avcodec_find_decoder(pCodecParameters->codec_id);
            qDebug()<<"Codec name "<<pLocalCodec->long_name;
            if (pLocalCodec->pix_fmts != NULL) {
                const AVPixelFormat *pxFormat = pLocalCodec->pix_fmts;
                do {
                    qDebug()<<"Pixel format"<<*pxFormat;
                } while (*(++pxFormat) != AV_PIX_FMT_NONE);
            } else {
                qDebug()<<"Pixel format == AV_PIX_FMT_NONE";
            }
            qDebug()<<"Codec frame size "<<pCodecParameters->frame_size;
            qDebug()<<"Codec frame width "<<pCodecParameters->width;
            qDebug()<<"Codec frame height "<<pCodecParameters->height;

            /*
             * Video stream was found. Complet serching.
             */
            break;
        }
    }

    pCodecContext = avcodec_alloc_context3(pLocalCodec);
    if (pCodecContext == NULL) {
        qDebug()<<"Can't allocate context 3";
        return FFmpegDecode::FFMPEG_ALLOCATE_CONTEXT3_ERROR;
    }
    if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0) {
        qDebug()<<"Parameter to context error";
        return FFmpegDecode::FFMPEG_PARAMETRS_TO_CONTEXT_ERROR;
    }
     pCodecContext->get_format = getFormat;
    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0) {
        qDebug()<<"Open codec error";
        return FFmpegDecode::FFMPEG_OPEN2_ERROR;
    }

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

FFmpegDecode::FFmpegStatus FFmpegDecode::readFrame(uint8_t *dstFrame)
{
    while(av_read_frame(pAVFormatContext, pkt) >= 0) {
        avcodec_send_packet(pCodecContext, pkt);
        if (avcodec_receive_frame(pCodecContext, frame) == 0) {
            //qDebug()<<"Frame Format"<<frame->format;
            //qDebug()<<"Frame Height"<<frame->height;
            //qDebug()<<"Frame Width"<<frame->width;
            //qDebug()<<"Frame Width"<<frame->buf[0]->size;
            //qDebug()<<"Frame Type (mpeg 4)"<<frame->pict_type;
            /*
             * Take a luminos data. On the YUV422 the luminouse is eac 2-th byte
             */
            for (uint32_t k = 0; k < 640 * 480 * 2; k += 2) {
                *dstFrame++ = frame->buf[0]->data[k];
            }
            //qDebug()<<"Copy ok";
            break;

        }
    }
    return FFmpegDecode::FFMPEG_OK;
}
