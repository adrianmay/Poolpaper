package com.didlio.android.poolpaper;

public class C {

    static {
        System.loadLibrary("poolpaperc");
    }
    public static native void init(int width, int height);
    public static native void step();
    public static native void bitmap(int id);

    
}
