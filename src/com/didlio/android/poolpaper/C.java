package com.didlio.android.poolpaper;

public class C {

    static {
        System.loadLibrary("poolpaperc");
    }
    public static native void init(int width, int height);
    public static native void step(long when);
    public static native void bitmap(int which, int id);

    
}
