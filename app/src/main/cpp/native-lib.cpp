#include <jni.h>
#include <string>
#include <android/bitmap.h>
#include <android/log.h>

extern "C" {
#include "gif/gif_lib.h"
}

#define  LOG_TAG    "native"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#define argb(a, r, g, b) (((a) & 0xff) << 24 | ((b) & 0xff) << 16 | ((g) & 0xff) << 8 | ((r) & 0xff))
#define dispose(ext) (((ext)->Bytes[0] & 0x1c) >> 2)
#define trans_index(ext) ((ext)->Bytes[3])
#define transparency(ext) ((ext)->Bytes[0] & 1)

struct GifBean {
    int next_frame;
    int total_frame;
    int *delays;
};

int drawFrame(GifFileType *gifFileType, AndroidBitmapInfo bitmapInfo, void *pixels);

int drawFrame(GifFileType *gif, AndroidBitmapInfo info, void *pixels, bool force_dispose_1);

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
    gifBean->next_frame = 0;
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
JNIEXPORT jint JNICALL
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
    void *pixels = nullptr;
    AndroidBitmap_lockPixels(env, bitmap, &pixels);

    // 绘制
    int delay = drawFrame(gifFileType, androidBitmapInfo, pixels, false);

    AndroidBitmap_unlockPixels(env, bitmap);

    auto *gifBean = (GifBean *) gifFileType->UserData;
    gifBean->next_frame++;
    if (gifBean->next_frame >= gifBean->total_frame - 1) {
        gifBean->next_frame = 0;
    }

    return delay;
}

int drawFrame(GifFileType *gifFileType, AndroidBitmapInfo bitmapInfo, void *pixels) {
    auto *gifBean = (GifBean *) gifFileType->UserData;
    // 压缩帧
    SavedImage savedImage = gifFileType->SavedImages[gifBean->next_frame];
    // 分为两部分，描述+像素
    GifImageDesc imageDesc = savedImage.ImageDesc;
    GifByteType *bits = savedImage.RasterBits;

    // bitmapInfo.width 表示图像像素，一个像素等于四个字节
    // bitmapInfo.stride 表示图像一行的字节数
    LOGD("bitmapInfo width:%d, height:%d, stride:%d", bitmapInfo.width, bitmapInfo.height,
         bitmapInfo.stride);
    LOGD("GifImageDesc Left:%d, Top:%d, Width:%d, Height:%d", imageDesc.Left, imageDesc.Top,
         imageDesc.Width, imageDesc.Height);

    int *px = (int *) pixels;
    int *line;
    int pointerPixel;
    for (int y = imageDesc.Top; y < imageDesc.Top + imageDesc.Height; ++y) {
        line = px;
        for (int x = imageDesc.Left; x < imageDesc.Left + imageDesc.Width; ++x) {
            // 每一行，计算出索引，即偏移
            pointerPixel = (y - imageDesc.Top) * imageDesc.Width + (x - imageDesc.Left);
            // 拿到压缩像素
            GifByteType gifByteType = bits[pointerPixel];
            // 查找字典，获取像素信息
            GifColorType gifColorType = imageDesc.ColorMap->Colors[gifByteType];
            // 赋值给像素，四个字节
            line[x] = argb(255, gifColorType.Red, gifColorType.Green, gifColorType.Blue);
        }
        // bitmapInfo.stride 表示一行的字节数，切到下一行
        px = reinterpret_cast<int *>((char *) px + bitmapInfo.stride);
    }

    return 33;
}

// 兼容 89a 87a 图片
// 89a 取自每一帧 SavedImage 里面的颜色字典
// 87a 取自外部 GifFileType 统一的颜色字典
int drawFrame(GifFileType *gif, AndroidBitmapInfo info, void *pixels, bool force_dispose_1) {
    GifColorType *bg;
    GifColorType *color;
    SavedImage *frame;
    ExtensionBlock *ext = 0;
    GifImageDesc *frameInfo;
    ColorMapObject *colorMap;
    int *line;
    int width, height, x, y, j, loc, n, inc, p;
    void *px;
    GifBean *gifBean = static_cast<GifBean *>(gif->UserData);
    width = gif->SWidth;
    height = gif->SHeight;
    frame = &(gif->SavedImages[gifBean->next_frame]);
    frameInfo = &(frame->ImageDesc);
    if (frameInfo->ColorMap) {
        colorMap = frameInfo->ColorMap;
    } else {
        colorMap = gif->SColorMap;
    }

    bg = &colorMap->Colors[gif->SBackGroundColor];

    for (j = 0; j < frame->ExtensionBlockCount; j++) {
        if (frame->ExtensionBlocks[j].Function == GRAPHICS_EXT_FUNC_CODE) {
            ext = &(frame->ExtensionBlocks[j]);
            break;
        }
    }
    // For dispose = 1, we assume its been drawn
    px = pixels;
    if (ext && dispose(ext) == 1 && force_dispose_1 && gifBean->next_frame > 0) {
        gifBean->next_frame = gifBean->next_frame - 1,
                drawFrame(gif, info, pixels, true);
    } else if (ext && dispose(ext) == 2 && bg) {
        for (y = 0; y < height; y++) {
            line = (int *) px;
            for (x = 0; x < width; x++) {
                line[x] = argb(255, bg->Red, bg->Green, bg->Blue);
            }
            px = (int *) ((char *) px + info.stride);
        }
    } else if (ext && dispose(ext) == 3 && gifBean->next_frame > 1) {
        gifBean->next_frame = gifBean->next_frame - 2,
                drawFrame(gif, info, pixels, true);
    }
    px = pixels;
    if (frameInfo->Interlace) {
        n = 0;
        inc = 8;
        p = 0;
        px = (int *) ((char *) px + info.stride * frameInfo->Top);
        for (y = frameInfo->Top; y < frameInfo->Top + frameInfo->Height; y++) {
            for (x = frameInfo->Left; x < frameInfo->Left + frameInfo->Width; x++) {
                loc = (y - frameInfo->Top) * frameInfo->Width + (x - frameInfo->Left);
                if (ext && frame->RasterBits[loc] == trans_index(ext) && transparency(ext)) {
                    continue;
                }
                color = (ext && frame->RasterBits[loc] == trans_index(ext)) ? bg
                                                                            : &colorMap->Colors[frame->RasterBits[loc]];
                if (color)
                    line[x] = argb(255, color->Red, color->Green, color->Blue);
            }
            px = (int *) ((char *) px + info.stride * inc);
            n += inc;
            if (n >= frameInfo->Height) {
                n = 0;
                switch (p) {
                    case 0:
                        px = (int *) ((char *) pixels + info.stride * (4 + frameInfo->Top));
                        inc = 8;
                        p++;
                        break;
                    case 1:
                        px = (int *) ((char *) pixels + info.stride * (2 + frameInfo->Top));
                        inc = 4;
                        p++;
                        break;
                    case 2:
                        px = (int *) ((char *) pixels + info.stride * (1 + frameInfo->Top));
                        inc = 2;
                        p++;
                }
            }
        }
    } else {
        px = (int *) ((char *) px + info.stride * frameInfo->Top);
        for (y = frameInfo->Top; y < frameInfo->Top + frameInfo->Height; y++) {
            line = (int *) px;
            for (x = frameInfo->Left; x < frameInfo->Left + frameInfo->Width; x++) {
                loc = (y - frameInfo->Top) * frameInfo->Width + (x - frameInfo->Left);
                if (ext && frame->RasterBits[loc] == trans_index(ext) && transparency(ext)) {
                    continue;
                }
                color = (ext && frame->RasterBits[loc] == trans_index(ext)) ? bg
                                                                            : &colorMap->Colors[frame->RasterBits[loc]];
                if (color)
                    line[x] = argb(255, color->Red, color->Green, color->Blue);
            }
            px = (int *) ((char *) px + info.stride);
        }
    }
    GraphicsControlBlock gcb;//获取控制信息
    DGifSavedExtensionToGCB(gif, gifBean->next_frame, &gcb);
    int delay = gcb.DelayTime * 10; // gcb.DelayTime 单位10ms，转为ms，就要 *10
    LOGD("delay %d", delay);
    return delay; // ms
}

extern "C"
JNIEXPORT void JNICALL
Java_com_blood_giftool_GifHelper_nativeRelease(JNIEnv *env, jclass clazz, jlong ptr) {
    auto *gifFileType = (GifFileType *) ptr;
    if (gifFileType) {
        if (gifFileType->UserData) {
            free(gifFileType->UserData);
            gifFileType->UserData = nullptr;
        }
    }
}