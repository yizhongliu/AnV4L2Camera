//
// Created by Administrator on 2019/8/9.
//

#ifndef NE_PLAYER_1_JAVACALLHELPER_H
#define NE_PLAYER_1_JAVACALLHELPER_H

#include <jni.h>

class JavaCallHelper {
public:
    JavaCallHelper(JavaVM *javaVM_, JNIEnv *env_, jobject instance_);

    ~JavaCallHelper();

    void onDataCallback(unsigned char* buf, int len, int width, int height, int pixFormat);


private:
    JavaVM *javaVM;
    JNIEnv *env;
    jobject instance;
    jmethodID jDataCallback;
};


//extern "C"
//JNIEXPORT void JNICALL
//Java_com_netease_jnitest_MainActivity_invokeHelper(JNIEnv *env, jobject instance) {
//    jclass clazz = env->FindClass("com/netease/jnitest/Helper");
//    //获得具体的静态方法 参数3：签名（下方说明）
//    //如果不会填 可以使用javap
//    jmethodID staticMethod = env->GetStaticMethodID(clazz,"staticMethod","(Ljava/lang/String;IZ)V");
//    //调用静态方法
//    jstring staticStr= env->NewStringUTF("C++调用静态方法");
//    env->CallStaticVoidMethod(clazz,staticMethod,staticStr,1,1);
//
//    //获得构造方法 <init>：构造方法写法
//    jmethodID constructMethod = env->GetMethodID(clazz,"<init>","()V");
//    //创建对象
//    jobject  helper = env->NewObject(clazz,constructMethod);
//    jmethodID instanceMethod = env->GetMethodID(clazz,"instanceMethod","(Ljava/lang/String;IZ)V");
//    jstring instanceStr= env->NewStringUTF("C++调用实例方法");
//    env->CallVoidMethod(helper,instanceMethod,instanceStr,2,0);
//
//    //释放
//    env->DeleteLocalRef(clazz);
//    env->DeleteLocalRef(staticStr);
//    env->DeleteLocalRef(instanceStr);
//    env->DeleteLocalRef(helper);
//}

#endif //NE_PLAYER_1_JAVACALLHELPER_H
