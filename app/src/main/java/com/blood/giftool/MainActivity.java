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
            int delay = mGifHelper.updateFrame(mBitmap);
            binding.iv.setImageBitmap(mBitmap);
            mHandler.sendEmptyMessageDelayed(0, delay);
        }
    };

    private ActivityMainBinding binding;
    private String gifPath;
    private GifHelper mGifHelper;
    private Bitmap mBitmap;

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
    }

    private String copyAssetsGif() {
        String gifFileName = "gif_89a.gif";
        File dstFile = new File(getFilesDir(), gifFileName);
        FileUtil.copyAssets(this, gifFileName, dstFile);
        return dstFile.getAbsolutePath();
    }

    private void init() {
        gifPath = copyAssetsGif();
        binding.nativeLoad.setOnClickListener(v -> nativeLoadGif());
    }

    private void nativeLoadGif() {
        mGifHelper = GifHelper.loadGifPath(gifPath);
        int width = mGifHelper.getWidth();
        int height = mGifHelper.getHeight();
        Log.i(TAG, "nativeLoadGif: width " + width + " , height " + height);
        mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        mHandler.sendEmptyMessage(0);
    }

}