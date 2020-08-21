//
// Created by Administrator on 2019/8/9.
//

#include "JavaCallHelper.h"
#include <android/log.h>

//定义日志打印宏函数
#define	LOG_TAG	"JavaCallHelper"
#define ALOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ALOGW(...)  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define ALOGD(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

JavaCallHelper::JavaCallHelper(JavaVM *javaVM_, JNIEnv *env_, jobject instance_) {
    ALOGE("enter : %s", __FUNCTION__);
    this->javaVM = javaVM_;
    this->env = env_;
//    this->instance = instance_;//不能直接赋值！
    //一旦涉及到 jobject 跨方法、跨线程，需要创建全局引用
    this->instance = env->NewGlobalRef(instance_);
    jclass clazz = env->GetObjectClass(instance);
//    cd 进入 class所在的目录 执行： javap -s 全限定名,查看输出的 descriptor
//    xx\app\build\intermediates\classes\debug>javap -s com.netease.jnitest.Helper
    jDataCallback = env->GetMethodID(clazz, "postDataFromNative", "([BIII)V");

    ALOGE("leave: %s", __FUNCTION__);

}

JavaCallHelper::~JavaCallHelper() {
    javaVM = 0;
    env->DeleteGlobalRef(instance);
    instance = 0;
}


void JavaCallHelper::onDataCallback(unsigned char* buf, int len, int width, int height, int pixFormat) {
    JNIEnv *env = NULL;

    int status = javaVM->GetEnv((void**)&env, JNI_VERSION_1_4);
    if (status < 0) {
        javaVM->AttachCurrentThread(&env, NULL);
        ALOGE("AttachCurrentThread");
    }

    jbyteArray array = env->NewByteArray(len);
    jbyte* pByte;
    if (array == NULL) {
        ALOGE("onDataCallback NewByteArray fail");
        return;
    }

    pByte = new jbyte[len];
    if (pByte == NULL) {
        ALOGE("onDataCallback creat jbyte memory fail");
        return;
    }
    for (int i = 0; i < len; i++) {
        *(pByte + i) = *(buf + i);
    }

    env->SetByteArrayRegion(array, 0, len, pByte);

    ALOGE("CallVoidMethod");
    env->CallVoidMethod(instance, jDataCallback, array, width, height, pixFormat);

    env->DeleteLocalRef(array);
    delete  pByte;
    pByte = NULL;

    if (env->ExceptionCheck()) {
        ALOGW("An exception occurred while notifying an event.");
        env->ExceptionClear();
    }

    if (status < 0) {
        javaVM->DetachCurrentThread();
        ALOGE("DetachCurrentThread");
    }
}
