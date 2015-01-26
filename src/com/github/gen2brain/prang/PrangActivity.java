package com.github.gen2brain.prang;

import org.libsdl.app.SDLActivity;

public class PrangActivity extends SDLActivity {

    @Override
    protected String[] getLibraries() {
        return new String[] {
            "SDL2",
            "SDL2_image",
            "SDL2_mixer",
            "main"
        };
    }

}
