build-native && adb shell pm uninstall -k oc.gle && adb logcat -c &&  ant debug && adb install bin\SDLActivity-debug.apk && adb shell am start -n oc.gle/oc.gle.SDLActivity && adb logcat -s "SDL", "SDL/APP" *:W