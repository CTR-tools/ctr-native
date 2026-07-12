# CTR Native on Android

CTR Native builds as a sideloadable Android APK. The Android target preserves
the same game code and PS1 VRAM model as the desktop builds while using SDL3 and
OpenGL ES 3 at the host boundary.

## Requirements

- JDK 17
- Android SDK Platform 35
- Android NDK `27.0.12077973`
- CMake 3.22.1 from the Android SDK
- An OpenGL ES 3 capable Android device
- An Android-recognized gamepad; there is currently no touch-control overlay
- Your own NTSC-U retail CTR disc image in raw MODE2/2352 BIN format

The current native memory model is 32-bit, so the APK contains `armeabi-v7a`
and `x86` libraries only. It is intended for direct sideloading, not Play Store
distribution.

## Build

Install the required SDK, NDK, and CMake versions through Android Studio's SDK
Manager, then run:

```sh
cd android
./gradlew assembleDebug
```

The Gradle wrapper downloads the pinned Gradle version automatically. From the
repository root, the APK is written to:

```text
android/app/build/outputs/apk/debug/app-debug.apk
```

While still in the `android/` directory, install or update it with:

```sh
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

You can also open the `android/` directory as a project in Android Studio.

## Disc Setup

On first launch, select your own NTSC-U retail CTR BIN through the setup screen.
The image must be a single-track raw MODE2/2352 BIN whose data track begins at
byte zero. Cooked 2048-byte ISO images do not contain the XA/STR sector data the
game needs.

The launcher validates the raw sector layout, streams the selected image into
app-owned storage, and starts the game. It does not modify the selected file and
does not request broad storage or network permission. Android removes the
imported copy when the app is uninstalled, so it must be selected again after a
fresh install.

## Controllers

CTR Native uses SDL's Gamepad API and supports up to four recognized gamepads.
Bluetooth controllers and USB controllers connected through USB host/OTG can
both work when Android exposes them as standard gamepad devices. Pair or connect
the controller through Android; the app does not scan for Bluetooth devices and
does not require Bluetooth or USB permission.

SDL includes mappings for many common controllers. A device exposed only as an
unknown joystick, without an SDL gamepad mapping, will not be opened by the
current input path. Controllers can be connected or removed while the game is
running.

## Logs

Native output is mirrored to Logcat with the `CTR-Native` tag:

```sh
adb logcat -s CTR-Native
```
