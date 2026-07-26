# CTR Native Android Port

This project has been ported to Android using SDL3.

## Building

1.  **Clone with submodules**: 
    `git clone --recursive https://github.com/Simon358/ctr-native-android`
2.  **Open in Android Studio**: Open the `android` folder.
3.  **Install NDK**: Go to `Settings > SDK Tools` and install `NDK (Side by side)` and `CMake`.
4.  **Sync Gradle**: Click the "Elephant" icon.
5.  **Build & Run**: The project is configured for 32-bit (`armeabi-v7a`) to maintain compatibility with the game's memory model.

## Assets Setup (Modern Android)

The game requires retail NTSC-U assets to run. 

### Recommended: File Picker (Automatic Copy)
On first launch, the app will ask for **"All Files Access"** permission. After granting this:
1.  In the "Missing Assets" dialog, click **"Select File"**.
2.  Navigate to and select your `ctr-u.bin` or `BIGFILE.BIG` file.
3.  The app will **automatically copy** the asset to its internal storage (`/data/data/com.ctrnative/files/assets/`).
4.  **Restart the app**. Once copied, you no longer need the original file on your storage.

### Manual Setup (PC/Root)
Place your `ctr-u.bin` or extracted assets folder into:
`/storage/emulated/0/Android/data/com.ctrnative/files/assets/`

## Troubleshooting

-   **Black Screen/Crash**: Ensure you have the correct NTSC-U retail assets.
-   **Viewing Logs**: Run the following command to see detailed output and crash reports:
    `adb logcat -s CTR-Native:V CTRNativeInput:V SDL:V AndroidRuntime:E`
-   **OpenGL Errors**: This port requires **OpenGL ES 3.0**. 

## Controls

- **Gamepads**: Supports standard Android gamepads (Backbone One, Xbox, DualShock) via SDL3.
- **Touch Controls**: On-screen buttons are automatically enabled if no gamepad is detected.
- **Vibration**: Supports native gamepad rumble via SDL3.
