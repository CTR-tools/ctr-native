# CTR Native Android Port

This project has been ported to Android using SDL3.

## Building

1. Open the `android` folder in Android Studio.
2. Sync Gradle.
3. Build and run.

The build targets `armeabi-v7a` to maintain 32-bit compatibility with the game's memory model.

## Assets Setup (S21+ / Modern Android)

Due to Android security restrictions, the game looks for assets in its private external storage.

1. Install the app on your phone.
2. Create the following directory on your phone:
   `/storage/emulated/0/Android/data/com.ctrnative/files/assets/`
3. Place your `ctr-u.bin` (NTSC-U retail disc image) into that `assets` folder.
   The path should be:
   `.../Android/data/com.ctrnative/files/assets/ctr-u.bin`
4. Alternatively, you can place extracted assets (BIGFILE.BIG, etc.) in the same `assets` folder.

## Controls

- The port supports the **Backbone One** and other standard Android gamepads via SDL3's Gamepad API.
- Touch controls are currently not implemented (keyboard/gamepad only).

## Troubleshooting

Check Logcat with tag `CTR-Native` for any initialization errors.
