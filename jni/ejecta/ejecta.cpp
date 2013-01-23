
#include <string.h>
#include <jni.h>
#include <android/log.h>
#include <GLES/gl.h>
#include <string>
#include <set>
#include "EJApp.h"
#include "EJTexture.h"

#define  LOG_TAG    "ejecta"
#define  NSLog(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

extern "C" {
	jint JNI_OnLoad(JavaVM *vm, void *reserved) {
		return JNI_VERSION_1_4;
	}

    static bool s_is_resumed = false;
    static EJTexture* texture = new EJTexture();

    JNIEXPORT void JNICALL Java_com_impactjs_ejecta_EjectaRenderer_nativeCreated(JNIEnv* env, jobject thiz , jstring package_path, jint w, jint h)
    {

        NSLog("nativeCreated : %d, %d", w, h);
        const char *nativeString = (env)->GetStringUTFChars(package_path, 0);
        EJApp::instance()->init(nativeString, w, h);
    }

    JNIEXPORT void JNICALL Java_com_impactjs_ejecta_EjectaRenderer_nativeChanged(JNIEnv* env, jobject thiz , jint w, jint h)
    {
        EJApp::instance()->setScreenSize(w, h);
        NSLog("nativeChanged : %d, %d", w, h);
    }

    JNIEXPORT void JNICALL Java_com_impactjs_ejecta_EjectaRenderer_nativeRender(JNIEnv* env, jobject thiz)
    {

        if (s_is_resumed)
        {
            s_is_resumed = false;

        }

        EJApp::instance()->run();

    }

    JNIEXPORT void JNICALL Java_com_impactjs_ejecta_EjectaRenderer_nativeFinalize(JNIEnv* env, jobject thiz)
    {
        EJApp::finalize();
    }

    JNIEXPORT void JNICALL Java_com_impactjs_ejecta_EjectaRenderer_nativePause(JNIEnv* env, jobject thiz)
    {
        EJApp::instance()->pause();
    }

    JNIEXPORT void JNICALL Java_com_impactjs_ejecta_EjectaRenderer_nativeResume(JNIEnv* env, jobject thiz)
    {
        s_is_resumed = true;
        EJApp::instance()->resume();
    }

    JNIEXPORT void JNICALL Java_com_impactjs_ejecta_EjectaRenderer_nativeTouch(JNIEnv* env, jobject thiz, jint action, jint x, jint y)
    {
        // switch (action)
        // {
        // case 0: // ACTION_DOWN
        //     (*s_mouse_event_handler)(static_cast<s16>(x), static_cast<s16>(y));
        //     (*s_key_event_handler)(KEY_LBUTTON, true);
        //     break;

        // case 1: // ACTION_UP:
        //     (*s_mouse_event_handler)(static_cast<s16>(x), static_cast<s16>(y));
        //     (*s_key_event_handler)(KEY_LBUTTON, false);
        //     break;

        // case 2: // ACTION_MOVE:
        //     (*s_mouse_event_handler)(static_cast<s16>(x), static_cast<s16>(y));
        //     break;

        // default:
        //     break;
        // }
    }

    JNIEXPORT void JNICALL Java_com_impactjs_ejecta_EjectaRenderer_nativeOnKeyDown(JNIEnv* env, jobject thiz, jint key_code)
    {
        //callKeyEventHandler(static_cast<s16>(key_code), true);
    }

    JNIEXPORT void JNICALL Java_com_impactjs_ejecta_EjectaRenderer_nativeOnKeyUp(JNIEnv* env, jobject thiz, jint key_code)
    {
        //callKeyEventHandler(static_cast<s16>(key_code), false);
    }

    JNIEXPORT void JNICALL Java_com_impactjs_ejecta_EjectaRenderer_nativeOnSensorChanged(JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat z)
    {
	   //(*s_gravity_sensor_handler)(static_cast<r32>(x), static_cast<r32>(y), static_cast<r32>(z));
    }

}