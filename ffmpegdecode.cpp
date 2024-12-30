#include <QDebug>

extern "C" {
    #include "stdio.h"
}

#include "ffmpegdecode.h"

#define INBUF_SIZE 4096

void logCb(void* ptr, int logLevel, const char* errorStr, va_list vaList)
{
    qDebug()<<errorStr;
}

void handleError(int errCode)
{
    char errStr[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errCode, errStr, sizeof(errStr));
    qDebug()<<"ERROR: "<<errStr;
}

FFmpegDecode::FFmpegDecode(QObject *parent) : QObject(parent)
{
    qDebug()<<"Video version"<<avutil_version();
    qDebug()<<"Video version"<<avutil_license();
    qDebug()<<"Video version"<<av_version_info();
}

const char *FFmpegDecode::getFfmpegInfo()
{
    return av_version_info();
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
 * Typicaly linux returb v4l2 video stream
 */
FFmpegDecode::FFmpegStatus FFmpegDecode::openInputCameraStream(QSize frameResolution)
{
    std::string resolution = (QString::number(frameResolution.width()) + "x" + QString::number(frameResolution.height())).toStdString();

    inputFormat = av_find_input_format("v4l2"); // video for linux (v4l2) camera outputformat
    av_dict_set(&dictionaryOptions, "framerate", "30", 0);
    av_dict_set(&dictionaryOptions, "video_size", resolution.c_str(),  0);

    if (avformat_open_input(&rxStreamContext, cameraPath, inputFormat, &dictionaryOptions) != 0 ) {
        qDebug()<<"Can't connect camera";
        return FFmpegDecode::FFMPEG_OPEN_INPUT_PATH_STREAM_ERROR;
    } else {
        qDebug()<<rxStreamContext->iformat->long_name;
    }

    if (avformat_find_stream_info(rxStreamContext, NULL) < 0) {
        qDebug()<<"Can't find stream";
        return FFmpegDecode::FFMPEG_FIND_INPUT_STREAM_ERROR;
    }

    return FFmpegDecode::FFMPEG_OK;
}

#define FFMPEG_ERROR(err)                          \
    if ((err) < 0) {                               \
       char errBuff[152];                          \
       av_strerror(err, errBuff, sizeof(errBuff)); \
       qDebug()<<errBuff;                          \
    }

void FFmpegDecode::generateSdp()
{
    char sdpBuff[2048];

    av_sdp_create(&txRtpStreamContext, 1, sdpBuff, sizeof(sdpBuff));
    qDebug()<<sdpBuff;
    QFile sdpFile("/home/oleksandr/streamSdp.sdp");
    sdpFile.open(QIODevice::ReadWrite);
    sdpFile.write(sdpBuff);
    sdpFile.close();
}

/*
 * 1 - find the sub video stream index on the input stream
 * 2 - Find decoder for the stream under the index
 * 3 - Alocate memory for the decoder context
 * 4 - Copy decoder parameters to the context
 * 5 - Open decoder
 */
FFmpegDecode::FFmpegStatus FFmpegDecode::openDecoder()
{
    videoStreamInd = av_find_best_stream(rxStreamContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

    /*
     * Find and alocate decoder for the camera stream
     */
    if (videoStreamInd >= 0) {
        codecDecodeParameters = rxStreamContext->streams[videoStreamInd]->codecpar;
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

        if (avcodec_open2(codecDecodeContext, codecDecode, NULL) < 0) {
            qDebug()<<"Open codec error";
            return FFmpegDecode::FFMPEG_OPEN2_ERROR;
        }
    } else {
        return FFmpegDecode::FFMPEG_FIND_VIDE_STREAM_ERROR;
    }

    return FFmpegDecode::FFMPEG_OK;
}


/*
 * The method is:
 * - capture the video stream from the Web camera
 * - decode it to the frame
 * - - show freame on the screen
 * - encode frame to the HD264 stream
 * - save stream to the output file
 */
FFmpegDecode::FFmpegStatus FFmpegDecode::cameraRecord(QSize frameResolution, QString outFileName)
{
    QFile outFile(outFileName);
    outFile.open(QIODevice::ReadWrite);

    avdevice_register_all();

    /*
     * To decode the camera stream we need to now the pixel format that will be return the decoder. So that, first we need to find decoder for the camera stream
     */
    openInputCameraStream(frameResolution);
    openDecoder();
    openEncoder(frameResolution);
   /*
     * Init container for the camera stream
     */
    pkt = av_packet_alloc();
    if (pkt == NULL) {
        qDebug()<<"Can't allk paket";
        return FFmpegDecode::FFMPEG_ALLOCATE_PKT_ERROR;
    }

    /*
     * Init frame for Decode
     */
    frame = av_frame_alloc();
    if (pkt == NULL) {
        qDebug()<<"Can't allk paket";
        return FFmpegDecode::FFMPEG_ALLOCATE_FRAME_ERROR;
    }

    /*
     * Init container for the encoder
     */
    pktEncode = av_packet_alloc();
    if (pkt == NULL) {
        qDebug()<<"Can't allk paket";
        return FFmpegDecode::FFMPEG_ALLOCATE_PKT_ERROR;
    }

    /*
     * Init frame for Encode
     */
    frameEncode = av_frame_alloc();
    if (pkt == NULL) {
        qDebug()<<"Can't allk paket";
        return FFmpegDecode::FFMPEG_ALLOCATE_FRAME_ERROR;
    }
    frameEncode->format = codecEncodeContext->pix_fmt;
    frameEncode->width = codecEncodeContext->width;
    frameEncode->height = codecEncodeContext->height;

    if (av_frame_get_buffer(frameEncode, 0) < 0) {
        qDebug()<<"Could not allocate the video frame data";
        return FFmpegDecode::FFMPEG_FRAME_GET_BUFF_ERROR;
    }

    /*
     * Prepare file to record camera stream
     */
    cameraRecFile.setFileName(filePath);
    cameraRecFile.open(QIODevice::WriteOnly);

    return FFmpegDecode::FFMPEG_OK;
}

/*
 * The method is:
 * - capture the video stream from the Web camera
 * - decode it to the frame
 * - - show freame on the screen
 * - encode frame to the HD264 stream
 * - create RTP server and send stream
 */
FFmpegDecode::FFmpegStatus FFmpegDecode::cameraSreamNetwork(QSize frameResolution)
{
    //const char *url = "rtp://192.168.31.82:5004";
    const char *url = "rtp://192.168.31.217:5004";
    //const char *url = "rtp://127.0.0.1:5004";
    int result;
    FFmpegStatus ffmpegResult;

    avdevice_register_all();
    /*
     * The ffmpeg say that this calling is unnesesary. It only need to support old version of GnuTLS or OpenSSL libraries
     */
    result = avformat_network_init();

    /*
     * To decode the camera stream we need to now the pixel format that will be return the decoder. So that, first we need to find decoder for the camera stream
     */
    openInputCameraStream(frameResolution);
    openDecoder();
    openEncoder(frameResolution);
    if ((ffmpegResult = openOutputRtpStream(url)) != FFmpegDecode::FFMPEG_OK) {
        return ffmpegResult;
    }

    /*
     * Init container (packet) for the camera stream
     */
    pkt = av_packet_alloc();
    if (pkt == NULL) {
        qDebug()<<"Can't allk paket";
        return FFmpegDecode::FFMPEG_ALLOCATE_PKT_ERROR;
    }

    /*
     * Init frame for camea stream Decode
     */
    frame = av_frame_alloc();
    if (pkt == NULL) {
        qDebug()<<"Can't allk paket";
        return FFmpegDecode::FFMPEG_ALLOCATE_FRAME_ERROR;
    }

    /*
     * Init container for the encoder (*packet* in terms of ffmpeg)
     */
    pktEncode = av_packet_alloc();
    if (pkt == NULL) {
        qDebug()<<"Can't allk paket";
        return FFmpegDecode::FFMPEG_ALLOCATE_PKT_ERROR;
    }

    /*
     * Init frame for Encode
     */
    frameEncode = av_frame_alloc();
    if (pkt == NULL) {
        qDebug()<<"Can't allk paket";
        return FFmpegDecode::FFMPEG_ALLOCATE_FRAME_ERROR;
    }
    frameEncode->format = codecEncodeContext->pix_fmt;
    frameEncode->width = codecEncodeContext->width;
    frameEncode->height = codecEncodeContext->height;

    if (av_frame_get_buffer(frameEncode, 0) < 0) {
        qDebug()<<"Could not allocate the video frame data";
        return FFmpegDecode::FFMPEG_FRAME_GET_BUFF_ERROR;
    }

    return FFmpegDecode::FFMPEG_OK;
}

/*
 * Open H264 encoder
 *
 */
FFmpegDecode::FFmpegStatus FFmpegDecode:: openEncoder(QSize frameResolution)
{
    /*
     * Create and configure encoder
     */
    codecEncode= avcodec_find_encoder(AV_CODEC_ID_H264); // Find the codec by ID. Also we can find codec by the name with function avcodec_find_decoder_by_name(), for example "H264-MPEG-4"
    if (codecEncode == 0) {
        qDebug()<<"Can't find H264 encoder";
        return FFmpegDecode::FFMPEG_FIND_CODEC_DECODER_ERROR;
    }

    codecEncodeContext = avcodec_alloc_context3(codecEncode);
    if (codecEncodeContext == NULL) {
        qDebug()<<"Can't alock encode H264";
        return FFmpegDecode::FFMPEG_ALOCK_CODEC_DECODER_ERROR;
    }

    /*
     * Structure of the H264 stream is the next
     *
     * Stream => GOP_0, GOP_1, GOP_2, ..., GOP_N, ...
     *
     * GOP_N => I_B_B_P_B_B_P
     *
     * I - picture
     *
     * P - picture
     *
     * B - picture
     *
     * The upper level of the stream is a chain of the GOP.
     * GOP - (acronim) Groupe Of Pictures
     * GOP utilizes various types of compressed pictures.
     * At the head of the GOP is a picture compressed by the MPEG algo: I. These pictures are independent from other frames.
     * P - type calculated as the difference between uncompressed and previous pictures.
     * B - calculate as an interpolation between the previous P or I pictures and the next P or I pictures.
     *
     * FFmpeg allows flexible adjust the GOP:
     * gop_size - the total number of (P + B) pictures  between two I pictures
     * max_b_frames - the number of the B frames between I and P frames or P and P frames. If max_b_frames = 0, the H264 does not include B frames.
     * If max_b_frames = 0, gop_size = it is the number of P frames.
     * pix_fmt - the HD265 required the YUV420 input format, which is why it must be equal to AV_PIX_FMT_YUV420P
     */

    /* resolution must be a multiple of two */
    codecEncodeContext->width = frameResolution.width();
    codecEncodeContext->height = frameResolution.height();
    /* frames per second */
    /* put sample parameters */
    codecEncodeContext->bit_rate = 400000;
    codecEncodeContext->time_base = (AVRational){1, 30}; // It is a base time unit for the encoder: 1/30 of the seconds OR 1000 / 30 ms. In this units wil ba calculate frame->pts and frame dts
    codecEncodeContext->framerate = (AVRational){30, 1};
    codecEncodeContext->gop_size = 12;
    codecEncodeContext->max_b_frames = 0;
    codecEncodeContext->pix_fmt = AV_PIX_FMT_YUV420P;

    /*
     * Apply one of the standart preset for the HD. This preset desribe at the HD264 standart
     *
     * I selected the settings to provide minimum latency on the encoding process
     */
    if (codecEncode->id == AV_CODEC_ID_H264) {
        av_opt_set(codecEncodeContext->priv_data, "preset", "ultrafast", 0);
        av_opt_set(codecEncodeContext->priv_data, "tune", "zerolatency", 0);
        av_opt_set_int(codecEncodeContext->priv_data, "buffsize", 10000, 0);
    }

    /*
     * Initilise the context with settings and oprions fron pass to the last arg.
     *
     */
    if (avcodec_open2(codecEncodeContext, codecEncode, NULL) < 0) {
        qDebug()<<"Open codec error";
        return FFmpegDecode::FFMPEG_OPEN_CODEC_DECODER_ERROR;
    }

    return FFmpegDecode::FFMPEG_OK;
}

/**
 * Create and open video data stream over the RTP protcol based with usig the H264 codec
 * Very important!! To call this function firs we need to intit the H264 codec, becouse we copy
 * the custom settings of the codec to the stream settings.
 */
FFmpegDecode::FFmpegStatus FFmpegDecode::openOutputRtpStream(const char *url)
{
    AVStream *txRtpStream = NULL;
    int result;


    result = avformat_alloc_output_context2(&txRtpStreamContext, NULL, "rtp", url);
    qDebug()<<"URL: "<<txRtpStreamContext->url;

    if (result < 0) {
        qDebug()<<"Alloc output rtp context error";
        FFMPEG_ERROR(result);
        return FFmpegDecode::FFMPEG_RTP_ALLOC_CONTEXT_ERROR;
    }

    if (txRtpStreamContext == NULL) {
        qDebug()<<"Alloc output rtp context null error";
        FFMPEG_ERROR(result);
        return FFmpegDecode::FFMPEG_RTP_ALLOC_CONTEXT_NULL_ERROR;
    }

    result = avio_open(&txRtpStreamContext->pb, url, AVIO_FLAG_WRITE);
    if(result < 0) {
        qDebug()<<"Open rtp video out error";
        FFMPEG_ERROR(result);
        return FFmpegDecode::FFMPEG_RTP_OPEN_STREAM_ERROR;
    }

    /*
     * avformat_new_stream - create stream and return reherence to stream handler from the contest!!
     */
    txRtpStream = avformat_new_stream(txRtpStreamContext, NULL);
    av_opt_set(txRtpStreamContext, "rtpflags", "send_immediately", 0);
    av_opt_set(txRtpStreamContext, "rtpflags", "nobuffer", 0);

    /* codecEncodeContext - it is a context of encoder.
     * Apply encoder parametrs (encoder was initilise upper on the method *openEncoder*) for the stream properties.
     */
    result = avcodec_parameters_from_context(txRtpStream->codecpar, codecEncodeContext);
    if(result < 0) {
        qDebug()<<"Get codec parameters to stream error";
        FFMPEG_ERROR(result);
        return FFmpegDecode::FFMPEG_RTP_OPEN_STREAM_ERROR;
    }

    //txRtpStream->time_base = codecEncodeContext->time_base;

    qDebug()<<"Stream codec name"<<avcodec_get_name(txRtpStream->codecpar->codec_id);
    qDebug()<<"Stream codec frame height"<<txRtpStream->codecpar->height;
    qDebug()<<"Stream codec frame width"<<txRtpStream->codecpar->width;
    qDebug()<<"Stream codec bit_rate"<<txRtpStream->codecpar->bit_rate;
    qDebug()<<"Stream codec timebase: "<<txRtpStream->time_base.num<<"/"<<txRtpStream->time_base.den;

    if (txRtpStream == NULL) {
        qDebug()<<"Alloc output rtsp stream create error";
        return FFmpegDecode::FFMPEG_RTP_CREATE_STREAM_ERROR;
    }

    /*
     * As I understand, thiis flag tell coddec add some header to each video packet. We need it becouse we
     * push the video packet to rtp stream. The additional information on the head of the packet help the
     * rtp client to decode packets on the correct order.
     */

    if (txRtpStreamContext->oformat->flags & AVFMT_GLOBALHEADER) {
        codecEncodeContext->flags |=  AV_CODEC_FLAG_GLOBAL_HEADER;
        qDebug()<<"Coddec need to add global header to the packets";
    }



    result = avformat_write_header(txRtpStreamContext, NULL);
    if (result < 0) {
        qDebug()<<"Write output headr error";
        FFMPEG_ERROR(result);
        return FFmpegDecode::FFMPEG_RTP_WRITE_HEADR_ERROR;
    }


    generateSdp();

    return FFmpegDecode::FFMPEG_OK;
}

void FFmpegDecode::streamRtp(uint8_t *dstFrame)
{
    int ret = 0;
    static int64_t pts = 0;
    static int64_t duration = av_rescale_q(1, (AVRational){1, 30}, codecEncodeContext->time_base);
    int i, x, y;

    while(av_read_frame(rxStreamContext, pkt) >= 0) { // read packet (NOT FRAME !!) from the camera stream
        if (pkt->stream_index == videoStreamInd) {
            ret  = avcodec_send_packet(codecDecodeContext, pkt); // send (pass) packet (ecncoded video data from camera) to the codec driver
            if (ret < 0) {
                qDebug()<<"Error submitting a packet for decoding"<<av_err2str(ret);
                return; //FFmpegDecode::FFMPEG_OK;
            }
            if (avcodec_receive_frame(codecDecodeContext, frame) == 0) { // take decode camera frame
                //qDebug()<<"Is open" << avcodec_is_open(codecEncodeContext);

                /*
                 * The HD264 encoder expected strongly YUV420 input format with the frame rezolution equal to the
                 * encoder context. We set target camera rezolution upper. But we can't set
                 * the format of the camera output (typicaly linux return V4l2 stream). That is why if the farame format is not the YUV420, we must
                 * convert it to YUV420.
                 */
                if (frame->format != AV_PIX_FMT_YUV420P) {
                    /*
                     *  Use the sws_scale library to convert the frame from the input type (in case of linux it is V4l2) to the H264 compatible (YUV420) format.
                     *  The *scale* meaning change not only size, but also format.
                     */
                    SwsContext *swScaleContext = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format,
                                                                frameEncode->width, frameEncode->height, (AVPixelFormat)frameEncode->format,
                                                                0, NULL, NULL, NULL);
                    if ((ret = sws_scale_frame(swScaleContext, frameEncode, frame)) < 0) {
                        qDebug()<<"Convert frame error: "<<ret;
                        return;
                    }
                }

                /*
                 * Copy the grayscale part of the picture to the external buffer
                 */
                memcpy(dstFrame, frameEncode->data[0], frameEncode->width * frameEncode->height);

                frameEncode->pts = pts;
                pts += duration;

                /*
                 * Encode YUV420 frame to the H264 packet
                 * Usualy ffmpeg use two call to encode: send_frame, receive_packet
                 */
                ret = avcodec_send_frame(codecEncodeContext, frameEncode);
                if (ret < 0) {
                    qDebug() <<"Error sending a frame to the encoder: "<<av_err2str(ret)<< "   " <<frameEncode->pts<<frameEncode;
                }


                //pktEncode->pts = pktEncode->dts = pts;
                //pktEncode->pos = pts;
                //pktEncode->duration = 3000;//av_rescale(codecEncodeContext->framerate, codecEncodeContext->time_base, txRtpStreamContext->streams[0]->time_base);;
                //pktEncode->pts = av_rescale_q(pktEncode->pts, codecEncodeContext->time_base, txRtpStreamContext->streams[0]->time_base);
                // pktEncode->dts = av_rescale_q(pktEncode->dts, codecEncodeContext->time_base, txRtpStreamContext->streams[0]->time_base);

                while (ret >= 0) {
                    ret = avcodec_receive_packet(codecEncodeContext, pktEncode);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        //handleError(ret);
                        return;
                    } else if (ret < 0) {
                        fprintf(stderr, "Error during encoding\n");
                        exit(1);
                    }

                    pktEncode->pts = pktEncode->dts = pts;

                    pktEncode->pos = pts;
                    pktEncode->duration = 3000;//av_rescale(codecEncodeContext->framerate, codecEncodeContext->time_base, txRtpStreamContext->streams[0]->time_base);;
                    pktEncode->pts = av_rescale_q(pktEncode->pts, codecEncodeContext->time_base, txRtpStreamContext->streams[0]->time_base);
                    pktEncode->dts = av_rescale_q(pktEncode->dts, codecEncodeContext->time_base, txRtpStreamContext->streams[0]->time_base);
                    //qDebug()<<"paket pts: "<<pktEncode->pts;
                    //qDebug()<<"stream time base: "<<txRtpStreamContext->streams[0]->time_base.num<<"/"<<txRtpStreamContext->streams[0]->time_base.den;
                    //qDebug()<<"paket position: "<<pktEncode->pos;
                    //qDebug()<<"paket duration: "<<pktEncode->duration;
                    //qDebug()<<"paket time base num: "<<pktEncode->time_base.num;
                    //avformat_write_header(txRtpStreamContext, NULL);
                    ret = av_interleaved_write_frame(txRtpStreamContext, pktEncode);
                    if(ret < 0) {
                        qDebug()<<"Send frame error";
                        FFMPEG_ERROR(ret);
                        //return FFmpegDecode::FFMPEG_RTP_OPEN_STREAM_ERROR;
                    }
                    av_packet_unref(pktEncode);
                }
                break;
            }
        }
    }
    return;// FFmpegDecode::FFMPEG_OK;
}



void FFmpegDecode::encode(uint8_t *dstFrame)
{
    int ret = 0;
    static uint32_t pts = 0;
    int i, x, y;

    while(av_read_frame(rxStreamContext, pkt) >= 0) { // read stream
        if (pkt->stream_index == videoStreamInd) {
            ret  = avcodec_send_packet(codecDecodeContext, pkt); // send (pass) encodet data to the codec driver
            if (ret < 0) {
                qDebug()<<"Error submitting a packet for decoding"<<av_err2str(ret);
                return; //FFmpegDecode::FFMPEG_OK;
            }
            if (avcodec_receive_frame(codecDecodeContext, frame) == 0) {
                //qDebug()<<"Is open" << avcodec_is_open(codecEncodeContext);

                /*
                 * The HD264 encoder expected strongly YUV420 input format with the frame rezolution equal to the
                 * encoder context. We set target camera rezolution upper. But we can't set
                 * the format of the camera output (typicaly linux return V4l2 stream). That is why if the farame format is not the YUV420, we must
                 * convert it to YUV420.
                 */
                if (frame->format != AV_PIX_FMT_YUV420P) {
                    /*
                     *  Use the sws_scale library to convert the frame from the input type (in case of linux it is V4l2) to the H264 compatible (YUV420) format.
                     *  The *scale* meaning change not only size, but also format.
                     */
                    SwsContext *swScaleContext = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format,
                                                                frameEncode->width, frameEncode->height, (AVPixelFormat)frameEncode->format,
                                                                0, NULL, NULL, NULL);
                    if ((ret = sws_scale_frame(swScaleContext, frameEncode, frame)) < 0) {
                        qDebug()<<"Convert frame error "<<ret;
                        return;
                    }
                }
                memcpy(dstFrame, frameEncode->data[0], frameEncode->width * frameEncode->height);


                frameEncode->pts = pts++;

                /*
                 * Encode YUV420 frame to the H264 packet
                 * Usualy ffmpeg use two call to encode: send_frame, receive_packet
                 */
                ret = avcodec_send_frame(codecEncodeContext, frameEncode);
                if (ret < 0) {
                    qDebug() <<"Error sending a frame to the encoder: "<<av_err2str(ret)<< "   " <<frameEncode->pts<<frameEncode;
                }
                while (ret >= 0) {
                    ret = avcodec_receive_packet(codecEncodeContext, pktEncode);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        return;
                    else if (ret < 0) {
                        fprintf(stderr, "Error during encoding\n");
                        exit(1);
                    }
                    QByteArray temp = QByteArray::fromRawData((const char *)pktEncode->data, pktEncode->size);
                    cameraRecFile.write(temp);
                    av_packet_unref(pktEncode);
                }
                break;
            }
        }
    }
    return;// FFmpegDecode::FFMPEG_OK;
}

void FFmpegDecode::stopVideo()
{
    /*
     * flush the video stream
     */

    /* flush the encoder */
    //encodeTest(c, NULL, pkt, f);

    /* Add sequence end code to have a real MPEG file.
       It makes only sense because this tiny examples writes packets
       directly. This is called "elementary stream" and only works for some
       codecs. To create a valid file, you usually need to write packets
       into a proper file format or protocol; see mux.c.
     */
    /*
    if (codecEncode->id == AV_CODEC_ID_MPEG1VIDEO || codecEncode->id->id == AV_CODEC_ID_MPEG2VIDEO)
        fwrite(endcode, 1, sizeof(endcode), f);
    */
    cameraRecFile.close();
}

FFmpegDecode::FFmpegStatus FFmpegDecode::filePlay()
{
    avdevice_register_all();

    /*
     * Open source of input data
     */
    if (avformat_open_input(&rxStreamContext, filePath, NULL, NULL) != 0 ) {
        qDebug()<<"Can't connect file";
        return FFmpegDecode::FFMPEG_OPEN_FILE_ERROR;
    } else {
        qDebug()<<rxStreamContext->iformat->long_name;
    }

    if (avformat_find_stream_info(rxStreamContext, NULL) < 0) {
        qDebug()<<"Can't find stream";
        return FFmpegDecode::FFMPEG_FIND_STREAM_ERROR;
    }

    qDebug()<<"Strems number = "<<rxStreamContext->nb_streams;

    /*
     * The video container could contain multiple multimedia stream.
     * Take a index of video stream and found a suitable decoder
     */
    videoStreamInd = av_find_best_stream(rxStreamContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (videoStreamInd >= 0) {
        codecDecodeParameters = rxStreamContext->streams[videoStreamInd]->codecpar;
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

FFmpegDecode::FFmpegStatus FFmpegDecode::camerraPlay(QSize frameResolution)
{
    avdevice_register_all();

    /*
     * To decode the camera stream we need to now the pixel format that will be return the decoder. So that, first we need to find decoder for the camera stream
     */

    openInputCameraStream(frameResolution);
    openDecoder();

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
    if (rxStreamContext == NULL) {
        return QSize(0,0);
    }
    return QSize(rxStreamContext->streams[videoStreamInd]->codecpar->width,
                 rxStreamContext->streams[videoStreamInd]->codecpar->height);
}

FFmpegDecode::FFmpegStatus FFmpegDecode::readFrame(uint8_t *dstFrame)
{
    int ret = 0;

    /*
     * Read frame from the video stream
     */
    while(av_read_frame(rxStreamContext, pkt) >= 0) { // read stream

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

                /*
                 * Copy luminos data. On the YUV420
                 */
                AVFrame *frameConvert;
                SwsContext *swsContext  = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format,
                                                         frame->width, frame->height, AV_PIX_FMT_RGB24,
                                                         0, NULL, NULL, NULL);
                frameConvert = av_frame_alloc();
                sws_scale_frame(swsContext, frameConvert,frame);
                memcpy(dstFrame, frameConvert->buf[0]->data, frame->width * frame->height * 3);//frameConvert->buf[0]->size);
                av_frame_free(&frameConvert);
                sws_freeContext(swsContext);

                break;

            }
        }
    }
    return FFmpegDecode::FFMPEG_OK;
}
