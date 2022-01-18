/*
**
** Copyright (C) 2010 Moko365 Inc
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define	LOG_TAG	"V4LCAMERA"
//#include <utils/Log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

//#include <ui/PixelFormat.h>

#include "V4L2Camera.h"

#include <android/log.h>
#include <mutex>
#include <libyuv/convert.h>
#include <libyuv/convert_argb.h>
#include <libyuv.h>

//定义日志打印宏函数
#define ALOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ALOGW(...)  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGD(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#ifndef MAX
#define MAX(a, b) ({__typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b; })
#define MIN(a, b) ({__typeof__(a) _a = (a); __typeof__(b) _b = (b); _a < _b ? _a : _b; })
#endif

// This value is 2 ^ 18 - 1, and is used to clamp the RGB values before their ranges
// are normalized to eight bits.
static const int kMaxChannelValue = 262143;

static inline uint32_t YUV2RGB(int nY, int nU, int nV) {
    nY -= 16;
    nU -= 128;
    nV -= 128;
    if (nY < 0) nY = 0;

    // This is the floating point equivalent. We do the conversion in integer
    // because some Android devices do not have floating point in hardware.
    // nR = (int)(1.164 * nY + 2.018 * nU);
    // nG = (int)(1.164 * nY - 0.813 * nV - 0.391 * nU);
    // nB = (int)(1.164 * nY + 1.596 * nV);

    int nR = 1192 * nY + 1634 * nV;
    int nG = 1192 * nY - 833 * nV - 400 * nU;
    int nB = 1192 * nY + 2066 * nU;

    nR = MIN(kMaxChannelValue, MAX(0, nR));
    nG = MIN(kMaxChannelValue, MAX(0, nG));
    nB = MIN(kMaxChannelValue, MAX(0, nB));

    nR = (nR >> 10) & 0xff;
    nG = (nG >> 10) & 0xff;
    nB = (nB >> 10) & 0xff;

    return 0xff000000 | (nR << 16) | (nG << 8) | nB;
}

void *render_task_start(void *args) {
    ALOGE("enter: %s", __PRETTY_FUNCTION__);
    V4L2Camera *element = static_cast<V4L2Camera *>(args);
    element ->_start();
    return 0;//一定一定一定要返回0！！！
}


V4L2Camera::V4L2Camera()
	: start(0)
{
}

V4L2Camera::~V4L2Camera()
{
    std::lock_guard<std::mutex> lock(windowLock);
    if (window != 0) {
        ANativeWindow_release(window);
        window = 0;
    }

    setListener(0);
}

int V4L2Camera::Open(const char *filename,
                      unsigned int w,
                      unsigned int h,
                      unsigned int p)
{
    int ret;
    struct v4l2_format format;

    fd = open(filename, O_RDWR, 0);
    if (fd < 0) {
        ALOGE("Error opening device: %s", filename);
        return -1;
    }

    width = w;
    height = h;
    pixelformat = p;

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = pixelformat;

    // MUST set 
    format.fmt.pix.field = V4L2_FIELD_ANY;

    ret = ioctl(fd, VIDIOC_S_FMT, &format);
    if (ret < 0) {
        ALOGE("Unable to set format: %s", strerror(errno));
        return -1;
    }

    return 0;
}

void V4L2Camera::Close()
{
    close(fd);
}

int V4L2Camera::Init()
{
    ALOGD("V4L2Camera::Init()");
    int ret;
    struct v4l2_requestbuffers rb;

    start = false;

    /* V4L2: request buffers, only 1 frame */
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rb.memory = V4L2_MEMORY_MMAP;
    rb.count = 1;

    ret = ioctl(fd, VIDIOC_REQBUFS, &rb);
    if (ret < 0) {
        ALOGE("Unable request buffers: %s", strerror(errno));
        return -1;
    }

    /* V4L2: map buffer  */
    memset(&buf, 0, sizeof(struct v4l2_buffer));

    buf.index = 0;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
    if (ret < 0) {
        ALOGE("Unable query buffer: %s", strerror(errno));
        return -1;
    }

    /* Only map one */
    mem = (unsigned char *)mmap(0, buf.length, PROT_READ | PROT_WRITE, 
				MAP_SHARED, fd, buf.m.offset);
    if (mem == MAP_FAILED) {
        ALOGE("Unable map buffer: %s", strerror(errno));
        return -1;
    }

    /* V4L2: queue buffer */
    ret = ioctl(fd, VIDIOC_QBUF, &buf);

    return 0;
}

void V4L2Camera::Uninit()
{
    munmap(mem, buf.length);
    return ;
}

void V4L2Camera::StartStreaming()
{
    enum v4l2_buf_type type;
    int ret;

    if (start) return;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        ALOGE("Unable query buffer: %s", strerror(errno));
        return;
    }

    if (window != 0) {
        pthread_create(&pid_start, 0, render_task_start, this);
    }

    start = true;
}

void V4L2Camera::StopStreaming()
{
    enum v4l2_buf_type type;
    int ret;

    if (!start) return;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        ALOGE("Unable query buffer: %s", strerror(errno));
        return;
    }

    start = false;
}

int V4L2Camera::GrabRawFrame(void *raw_base)
{
    int ret;
    int data_size;

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    /* V4L2: dequeue buffer */
    ret = ioctl(fd, VIDIOC_DQBUF, &buf);
    if (ret < 0) {
        ALOGE("Unable query buffer: %s", strerror(errno));
        return ret;
    }
    ALOGD("copy size :%d", buf.bytesused);

    data_size = buf.bytesused;

    /* copy to userspace */
    memcpy(raw_base, mem,  buf.bytesused);

    /* V4l2: queue buffer again after that */
    ret = ioctl(fd, VIDIOC_QBUF, &buf);
    if (ret < 0) {
        ALOGE("Unable query buffer: %s", strerror(errno));
        return ret;
    }

    return data_size;
}

bool WRITE_FILE = false;
void V4L2Camera::Convert(void *r, void *p, unsigned int rSize)
{
    unsigned char *raw = (unsigned char *)r;
    unsigned char *preview = (unsigned char *)p;

    ALOGE("Convert rSize: %d" , rSize);

    /* TODO: Convert YUYV to ARGB. */
    if (pixelformat == V4L2_PIX_FMT_YUYV) {
        int size = width * height * 2;
        int in;
        int out;

        unsigned char y1;
        unsigned char u;
        unsigned char y2;
        unsigned char v;

        uint32_t argb;
        for(in = 0, out = 0; in < size; in += 4, out += 8) {
            y1 = raw[in];
            u = raw[in + 1];
            y2 = raw[in + 2];
            v = raw[in + 3];

            //android　ARGB_8888 像素数据在内存中其实是以R G B A R G B A …的顺序排布的
            argb = YUV2RGB(y1,u,v);
            preview[out] = (argb >> 16) & 0xff;;
            preview[out + 1] = (argb >> 8) & 0xff;
            preview[out + 2] = argb & 0xff;
            preview[out + 3] = 0xff;

            argb = YUV2RGB(y2,u,v);
            preview[out + 4] = (argb >> 16) & 0xff;
            preview[out + 5] = (argb >> 8) & 0xff;
            preview[out + 6] = argb & 0xff;
            preview[out + 7] = 0xff;
        }
    } else if (pixelformat == V4L2_PIX_FMT_MJPEG) {
        int src_width = 0;
        int src_height = 0;

        libyuv::MJPGSize(raw, rSize, &src_width, &src_height);

        //经图片保存，16进制查看保存的改格式为   64 82 35 ff    --   B G R A
        //stride 跨距, 它描述一行像素中, 该颜色分量所占的 byte 数目
        libyuv::MJPGToARGB(raw, rSize, preview, width * 4, src_width,
                                 src_height, width, height);

#ifdef SAVE_JPEG
        if (!WRITE_FILE) {
            const char *path = "/sdcard/argb.bmp"; // 路径
            bmp_write(preview, width, height, path);
            WRITE_FILE = true;
        }
#endif

        //WINDOW_FORMAT_RGBA_8888  排列顺序为 R G B A
        unsigned char temp;
        for (int i = 0; i < width * height * 4; i = i + 4) {
            temp = preview[i+2];
            preview[i+2] = preview[i];
            preview[i] = temp;
        }


        ALOGE("MJPGToARGB: %d, %d", src_width, src_height);
    }


    return;
}


std::list<Parameter> V4L2Camera::getParameters() {
    struct v4l2_fmtdesc fmtd;	//存的是摄像头支持的传输格式
    struct v4l2_frmsizeenum  frmsize;	//存的是摄像头对应的图片格式所支持的分辨率
    struct v4l2_frmivalenum  framival;	//存的是对应的图片格式，分辨率所支持的帧率
    Parameter parameter;
    Frame frame;

    parameters.clear();

    for (int i = 0; ; i++)
    {
        fmtd.index = i;
        fmtd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_ENUM_FMT, &fmtd) < 0)
            break;
        ALOGD("fmt %d: %s\n", i, fmtd.description);
        parameter.pixFormat = fmtd.pixelformat;
        parameter.frames.clear();
        // 查询这种图像数据格式下支持的分辨率
        for (int j = 0; ; j++)
        {
            frmsize.index = j;
            frmsize.pixel_format = fmtd.pixelformat;
            if (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) < 0)
                break;
            ALOGD("w = %d, h = %d \n", frmsize.discrete.width, frmsize.discrete.height);

            frame.width = frmsize.discrete.width;
            frame.height = frmsize.discrete.height;
            frame.frameRate.clear();

            //查询在这种图像数据格式下这种分辨率支持的帧率
            for (int k = 0; ; k++)
            {
                framival.index = k;
                framival.pixel_format = fmtd.pixelformat;
                framival.width = frmsize.discrete.width;
                framival.height = frmsize.discrete.height;
                if (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &framival) < 0)
                    break;
                //下面是帧率的获取
                FrameRate frameRate;
                frameRate.numerator = framival.discrete.numerator;
                frameRate.denominator = framival.discrete.denominator;
                frame.frameRate.push_back(frameRate);
                ALOGD("frame interval: %d, %d\n", framival.discrete.numerator, framival.discrete.denominator);
            }

            parameter.frames.push_back(frame);
        }

        parameters.push_back(parameter);
    }

    return parameters;

}

int V4L2Camera::setPreviewSize(int width, int height, int pixformat) {
    int ret;
    struct v4l2_format format;

    ALOGD("setPreviewSize %d, %d, %d", width, height, pixformat);


    this->width = width;
    this->height = height;
    this->pixelformat = pixformat;

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = pixelformat;

    // MUST set
    format.fmt.pix.field = V4L2_FIELD_ANY;

    ret = ioctl(fd, VIDIOC_S_FMT, &format);
    if (ret < 0) {
        ALOGE("Unable to set format: %s", strerror(errno));
        return -1;
    }

    return 0;
}

void V4L2Camera::setSurface(ANativeWindow *window) {
    std::lock_guard<std::mutex> lock(windowLock);
    if (this->window != 0) {
        ANativeWindow_release(this->window);
    }

    this->window = window;
}

void V4L2Camera::_start() {
    unsigned char *raw = new unsigned char[buf.length];
    ALOGE("_start raw buf.length %d", buf.length);
    unsigned char *preview = new unsigned char[width * height * 4]; //RGBA的大小

    int size;
    int format = -1;
    while (start) {
        size = GrabRawFrame(raw);
        if (size < 0) {
            usleep(1000);
            continue;
        }

        sendDataToJava(raw, size);

        Convert(raw, preview, size);

        renderVideo(preview);
    }

    delete[] raw;
    delete[] preview;
}

void V4L2Camera::renderVideo(unsigned char *preview) {
    std::lock_guard<std::mutex> lock(windowLock);
    if (window == 0) {
        return;
    }
    ALOGE("RenderVideoElement width:%d, height:%d", width,height);
    ANativeWindow_setBuffersGeometry(window, width,
                                     height,
                                     WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
        return;
    }
    //把buffer中的数据进行赋值（修改）
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    memcpy(dst_data, preview, width*height*4);

    ANativeWindow_unlockAndPost(window);

}

void V4L2Camera::setListener(JavaCallHelper *listener) {
    std::lock_guard<std::mutex> lock(listenerLock);
    if (this->listener != 0) {
        delete this->listener;
    }
    this->listener = listener;
}

void V4L2Camera::sendDataToJava(unsigned char *raw, int size) {
    std::lock_guard<std::mutex> lock(listenerLock);
    int format = -1;

    switch (pixelformat) {
        case V4L2_PIX_FMT_YUYV:
            format = YUYV;
            break;
        case V4L2_PIX_FMT_MJPEG:
            format = MJPEG;
            break;
    }

    ALOGD("pixFormat %d, size : %d  ", pixelformat, size);
    if (listener != 0 && size != 0) {
        listener->onDataCallback(raw, size, width, height, format);
    }
}

int V4L2Camera::bmp_write(unsigned char *image, int imageWidth, int imageHeight, const char *filename)
{
    /*
     *  bmp文件头(14 bytes) + 位图信息头(40 bytes) + 调色板(由颜色索引数决定) + 位图数据(由图像尺寸决定)
     *  0x42, 0x4d  fileType -- 相对String "BM"
     *  0, 0, 0, 0  -- The size of the BMP file in bytes
     *  0, 0, 0, 0  -- Reserved
     *  54, 0, 0, 0 -- The offset, i.e. starting address, of the byte where the bitmap image data (pixel array) can be found,
     *  40, 0, 0, 0 -- The size of this header (12 bytes)
     *  0, 0, 0, 0  -- The bitmap width in pixels
     *  0, 0, 0, 0  -- The bitmap height in pixels
     *  1, 0 -- The number of color planes, must be 1
     *  32, 0 -- The number of bits per pixel 即 RGBA 8888
     * */
    unsigned char header[54] = {
            0x42, 0x4d, 0, 0, 0, 0, 0, 0, 0, 0,
            54, 0, 0, 0, 40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 32, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0
    };

    long file_size = (long)imageWidth * (long)imageHeight * 4 + 54;
    header[2] = (unsigned char)(file_size &0x000000ff);
    header[3] = (file_size >> 8) & 0x000000ff;
    header[4] = (file_size >> 16) & 0x000000ff;
    header[5] = (file_size >> 24) & 0x000000ff;

    long width = imageWidth;
    header[18] = width & 0x000000ff;
    header[19] = (width >> 8) &0x000000ff;
    header[20] = (width >> 16) &0x000000ff;
    header[21] = (width >> 24) &0x000000ff;

    long height = imageHeight;
    header[22] = height &0x000000ff;
    header[23] = (height >> 8) &0x000000ff;
    header[24] = (height >> 16) &0x000000ff;
    header[25] = (height >> 24) &0x000000ff;

    char fname_bmp[128];
    sprintf(fname_bmp, "%s", filename);

    FILE *fp;
    if (!(fp = fopen(fname_bmp, "wb")))
        return -1;

    fwrite(header, sizeof(unsigned char), 54, fp);
    fwrite(image, sizeof(unsigned char), (size_t)(long)imageWidth * imageHeight * 4, fp);

    fclose(fp);
    return 0;
}



