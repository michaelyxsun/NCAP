package com.msun.ncap;

import android.app.NativeActivity;
import android.os.Bundle;

import com.msun.ncap.databinding.ActivityMainBinding;

public class MainActivity extends NativeActivity {
    static {
        System.loadLibrary("main");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }
}
