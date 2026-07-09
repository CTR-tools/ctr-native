package com.ctrnative;

import org.libsdl.app.SDLActivity;

public class CTRNativeActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {
            "SDL3",
            "ctr_native"
        };
    }
}
