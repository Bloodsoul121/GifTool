#include <jni.h>
#include <string>
#include <android/bitmap.h>
#include <android/log.h>

extern "C" {
#include "gif/gif_lib.h"
}

#define  LOG_TAG    "native"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

#define argb(a, r, g, b) (((a) & 0xff) << 24 | ((b) & 0xff) << 16 | ((g) & 0xff) << 8 | ((r) & 0xff))

struct GifBean {
    int current_frame;
    int total_frame;
    int *delays;
};

void drawFrame(GifFileType *gifFileType, AndroidBitmapInfo bitmapInfo, void *pixels);

extern "C"
JNIEXPORT jlong JNICALL
Java_com_blood_giftool_GifHelper_nativeLoadGifPath(JNIEnv *env, jclass clazz, jstring _path) {
    // 转C类型
    const char *path = env->GetStringUTFChars(_path, nullptr);

    int error; // 打开成功或失败
    GifFileType *gifFileType = DGifOpenFileName(path, &error);

    // 初始化缓冲区 SavedImages
    DGifSlurp(gifFileType);

    auto *gifBean = (GifBean *) malloc(sizeof(GifBean));
    memset(gifBean, 0, sizeof(GifBean));
    gifBean->current_frame = 0;
    gifBean->total_frame = gifFileType->ImageCount;
    gifFileType->UserData = gifBean;

    // 释放
    env->ReleaseStringUTFChars(_path, path);

    return reinterpret_cast<jlong>(gifFileType);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_blood_giftool_GifHelper_nativeGetWith(JNIEnv *env, jclass clazz, jlong ptr) {
    auto *gifFileType = (GifFileType *) ptr;
    return gifFileType->SWidth;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_blood_giftool_GifHelper_nativeGetHeight(JNIEnv *env, jclass clazz, jlong ptr) {
    auto *gifFileType = (GifFileType *) ptr;
    return gifFileType->SHeight;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_blood_giftool_GifHelper_nativeUpdateFrame(JNIEnv *env, jclass clazz, jlong ptr,
                                                   jobject bitmap) {
    auto *gifFileType = (GifFileType *) ptr;
    int width = gifFileType->SWidth;
    int height = gifFileType->SHeight;
    LOGD("gifFileType : width %d height %d", width, height);

    AndroidBitmapInfo androidBitmapInfo;
    AndroidBitmap_getInfo(env, bitmap, &androidBitmapInfo);
    int _width = androidBitmapInfo.width;
    int _height = androidBitmapInfo.height;
    LOGD("androidBitmapInfo : width %d height %d", _width, _height);

    // 将bitmap转为一个二位数组
    void *pixels;
    AndroidBitmap_lockPixels(env, bitmap, &pixels);

    // 绘制
    drawFrame(gifFileType, androidBitmapInfo, pixels);

    AndroidBitmap_unlockPixels(env, bitmap);

    auto *gifBean = (GifBean *) gifFileType->UserData;
    gifBean->current_frame++;
    if (gifBean->current_frame >= gifBean->total_frame - 1) {
        gifBean->current_frame = 0;
    }
}

void drawFrame(GifFileType *gifFileType, AndroidBitmapInfo bitmapInfo, void *pixels) {
    auto *gifBean = (GifBean *) gifFileType->UserData;
    // 压缩帧
    SavedImage savedImage = gifFileType->SavedImages[gifBean->current_frame];
    // 分为两部分，描述+像素
    GifImageDesc imageDesc = savedImage.ImageDesc;
    GifByteType *bits = savedImage.RasterBits;

    // bitmapInfo.width 表示图像像素
    LOGD("drawFrame width:%d, height:%d, stride:%d", bitmapInfo.width, bitmapInfo.height, bitmapInfo.stride);

    int *px = (int *) pixels;
    int *line;
    int pointerPixel;
    for (int y = imageDesc.Top; y < imageDesc.Top + imageDesc.Height; ++y) {
        line = px;
        for (int x = imageDesc.Left; x < imageDesc.Left + imageDesc.Width; ++x) {
            // 每一行，计算出索引，即偏移
            pointerPixel = (y - imageDesc.Top) * imageDesc.Width + (x - imageDesc.Left);
            GifByteType gifByteType = bits[pointerPixel];
            // 查找字典
            GifColorType gifColorType = imageDesc.ColorMap->Colors[gifByteType];
            // 赋值
            line[x] = argb(255, gifColorType.Red, gifColorType.Green, gifColorType.Blue);
        }
        // bitmapInfo.stride 表示一行的字节数，切到下一行
        px = reinterpret_cast<int *>((char *) px + bitmapInfo.stride);
    }

}