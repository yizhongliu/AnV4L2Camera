/*
**
** Copyright (C) 2010 Moko365 Inc.
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

#ifndef _CAMIF_H_
#define _CAMIF_H_

#include <linux/videodev2.h>
#include <list>
#include <mutex>
#include <android/native_window.h>
#include "JavaCallHelper.h"

//sync with com.iview.common.module.ImageUtils
#define YV12 0
#define NV21 1
#define YUYV 2
#define MJPEG 3
#define H264 4


typedef struct {
    int numerator;
    int denominator;
}FrameRate;

typedef struct {
	int width;
	int height;
	std::list<FrameRate> frameRate;
} Frame;

typedef struct {
	int pixFormat;
	std::list<Frame> frames;
}Parameter;

#define STATE_OK 0
#define ERROR_INIT_FAIL  -1
#define ERROR_LOGIN_FAIL  -2
#define ERROR_STATE_ILLEGAL -3
#define ERROR_CAPABILITY_UNSUPPORT  -4
#define ERROR_OPEN_FAIL  -5
#define ERROR_PREVIEW_FAIL  -6



class V4L2Camera {

public:
    V4L2Camera();
    ~V4L2Camera();

    int Open(const char *device,
	     unsigned int width,
	     unsigned int height,
	     unsigned int pixelformat);
    int Init();
    void Uninit();
    void Close();

    void StartStreaming();
    void StopStreaming();

	std::list<Parameter> getParameters();
	int setPreviewSize(int width, int height, int pixformat);

    int GrabRawFrame(void *raw_base);
    void Convert(void *raw_base,
		 void *preview_base,
		 unsigned int rawSize);

    void setSurface(ANativeWindow *window);

    void _start();
    void renderVideo(unsigned char *preview);

    void setListener(JavaCallHelper * listener);
    void sendDataToJava(unsigned char *raw, int dataSize);

	int bmp_write(unsigned char *image, int imageWidth, int imageHeight, const char *filename);

private:
    int fd;
    int start;
    unsigned char *mem;
    struct v4l2_buffer buf;

    unsigned int width;
    unsigned int height;
    unsigned int pixelformat;

    std::list<Parameter> parameters;

    ANativeWindow *window = 0;
    std::mutex windowLock;

    pthread_t pid_start;

    JavaCallHelper* listener = 0;
	std::mutex listenerLock;
};


#endif
