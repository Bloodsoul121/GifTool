package com.blood.giftool;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.blood.giftool.databinding.ActivityMainBinding;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";

    static {
        System.loadLibrary("native-lib");
    }

    private final Handler mHandler = new Handler(Looper.getMainLooper()) {
        @Override
        public void handleMessage(@NonNull Message msg) {
            switch (msg.what) {
                case 0:
                    int delay1 = mGifHelper87a.updateFrame(mBitmap87a);
                    binding.iv87a.setImageBitmap(mBitmap87a);
                    mHandler.sendEmptyMessageDelayed(0, delay1);
                    break;
                case 1:
                    int delay2 = mGifHelper89a.updateFrame(mBitmap89a);
                    binding.iv89a.setImageBitmap(mBitmap89a);
                    mHandler.sendEmptyMessageDelayed(1, delay2);
                    break;
            }

        }
    };

    private ActivityMainBinding binding;
    private String mGifPath87a;
    private String mGifPath89a;
    private GifHelper mGifHelper87a;
    private GifHelper mGifHelper89a;
    private Bitmap mBitmap87a;
    private Bitmap mBitmap89a;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        init();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mHandler.removeCallbacksAndMessages(null);
        if (mGifHelper87a != null) {
            mGifHelper87a.release();
        }
        if (mGifHelper89a != null) {
            mGifHelper89a.release();
        }
    }

    private String copyAssetsGif(String gifFileName) {
        File dstFile = new File(getFilesDir(), gifFileName);
        FileUtil.copyAssets(this, gifFileName, dstFile);
        return dstFile.getAbsolutePath();
    }

    private void init() {
        mGifPath87a = copyAssetsGif("gif_87a.gif");
        mGifPath89a = copyAssetsGif("gif_89a.gif");
        binding.nativeLoad87a.setOnClickListener(v -> nativeLoadGif87a());
        binding.nativeLoad89a.setOnClickListener(v -> nativeLoadGif89a());
    }

    private void nativeLoadGif87a() {
        mGifHelper87a = GifHelper.loadGifPath(mGifPath87a);
        int width = mGifHelper87a.getWidth();
        int height = mGifHelper87a.getHeight();
        Log.i(TAG, "nativeLoadGif87a: width " + width + " , height " + height);
        mBitmap87a = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        mHandler.removeMessages(0);
        mHandler.sendEmptyMessage(0);
    }

    private void nativeLoadGif89a() {
        mGifHelper89a = GifHelper.loadGifPath(mGifPath89a);
        int width = mGifHelper89a.getWidth();
        int height = mGifHelper89a.getHeight();
        Log.i(TAG, "nativeLoadGif89a: width " + width + " , height " + height);
        mBitmap89a = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        mHandler.removeMessages(1);
        mHandler.sendEmptyMessage(1);
    }

}