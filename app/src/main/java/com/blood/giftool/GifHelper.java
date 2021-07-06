package com.blood.giftool;

import android.graphics.Bitmap;

public class GifHelper {

    private final long mPtr;

    private GifHelper(long ptr) {
        mPtr = ptr;
    }

    public static GifHelper loadGifPath(String path) {
        long ptr = nativeLoadGifPath(path);
        return new GifHelper(ptr);
    }

    public int getWidth() {
        return nativeGetWith(mPtr);
    }

    public int getHeight() {
        return nativeGetHeight(mPtr);
    }

    public int updateFrame(Bitmap bitmap) {
        nativeUpdateFrame(mPtr, bitmap);
        return 33;
    }

    public native static long nativeLoadGifPath(String path);

    public native static int nativeGetWith(long ptr);

    public native static int nativeGetHeight(long ptr);

    public native static void nativeUpdateFrame(long ptr, Bitmap bitmap);

}
