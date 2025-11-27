#include <jni.h>
#include <dlfcn.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "webp/demux.h" 
#include "webp/decode.h"
#include "webp/encode.h"

typedef struct {
    WebPData data;
    WebPDemuxer* demux;
	// WebPAnimDecoder* anim_decoder;
	// int canvas_width, canvas_height;
} WebPAnim;

static void* handle_sharpyuv = NULL;
static void* handle_webp     = NULL;
static void* handle_demux    = NULL;

// webp demux symbols
static WebPDemuxer* (*FUNC_WebPDemuxInternal)(const WebPData*, int, WebPDemuxState*, int) = NULL;
static void (*FUNC_WebPDemuxDelete)(WebPDemuxer*) = NULL;
static uint32_t (*FUNC_WebPDemuxGetI)(const WebPDemuxer*, WebPFormatFeature) = NULL;
static int (*FUNC_WebPDemuxGetFrame)(const WebPDemuxer*, int, WebPIterator*) = NULL;
static int (*FUNC_WebPDemuxNextFrame)(WebPIterator*) = NULL;
static void (*FUNC_WebPDemuxReleaseIterator)(WebPIterator*) = NULL;

static int (*FUNC_WebPInitDecoderConfigInternal)(WebPDecoderConfig*, int) = NULL;
static VP8StatusCode (*FUNC_WebPDecode)(const uint8_t*, size_t, WebPDecoderConfig*) = NULL;

// more demux symbols
static int (*FUNC_WebPAnimDecoderOptionsInitInternal)(WebPAnimDecoderOptions*, int) = NULL;
static void (*FUNC_WebPAnimDecoderDelete)(WebPAnimDecoder* dec) = NULL;
static int (*FUNC_WebPAnimDecoderGetInfo)(const WebPAnimDecoder* dec, WebPAnimInfo* info) = NULL;
static WebPAnimDecoder* (*FUNC_WebPAnimDecoderNewInternal)(const WebPData*, 
							const WebPAnimDecoderOptions*, int) = NULL; 

// webp decode symbol
static uint8_t* (*FUNC_WebPDecodeRGBAInto)(const uint8_t*, size_t, uint8_t*, size_t, int) = NULL;

JNIEXPORT jboolean JNICALL
Java_com_mycompany_myndkapp_HelloJni_nativeInit(JNIEnv* env, jclass clazz,
        jstring jsh, jstring jw, jstring jd)
{
    const char* psh = (*env)->GetStringUTFChars(env, jsh, 0);
    const char* pw  = (*env)->GetStringUTFChars(env, jw, 0);
    const char* pd  = (*env)->GetStringUTFChars(env, jd, 0);

    handle_sharpyuv = dlopen(psh, RTLD_NOW);
    handle_webp     = dlopen(pw, RTLD_NOW);
    handle_demux    = dlopen(pd, RTLD_NOW);

    (*env)->ReleaseStringUTFChars(env, jsh, psh);
    (*env)->ReleaseStringUTFChars(env, jw, pw);
    (*env)->ReleaseStringUTFChars(env, jd, pd);

    if (!handle_sharpyuv || !handle_webp || !handle_demux) return JNI_FALSE;

    // DEMUX
    FUNC_WebPDemuxInternal = dlsym(handle_demux, "WebPDemuxInternal");
    FUNC_WebPDemuxDelete   = dlsym(handle_demux, "WebPDemuxDelete");
    FUNC_WebPDemuxGetI     = dlsym(handle_demux, "WebPDemuxGetI");
    FUNC_WebPDemuxGetFrame = dlsym(handle_demux, "WebPDemuxGetFrame");
    FUNC_WebPDemuxNextFrame = dlsym(handle_demux, "WebPDemuxNextFrame");
    FUNC_WebPDemuxReleaseIterator = dlsym(handle_demux, "WebPDemuxReleaseIterator");
	
	FUNC_WebPInitDecoderConfigInternal = dlsym(handle_demux, "WebPInitDecoderConfigInternal");
	FUNC_WebPDecode = dlsym(handle_demux, "WebPDecode");
	
	FUNC_WebPAnimDecoderOptionsInitInternal = dlsym(handle_demux, "WebPAnimDecoderOptionsInitInternal");
	FUNC_WebPAnimDecoderNewInternal = dlsym(handle_demux, "WebPAnimDecoderNewInternal");
	FUNC_WebPAnimDecoderGetInfo = dlsym(handle_demux, "WebPAnimDecoderGetInfo");
	FUNC_WebPAnimDecoderDelete = dlsym(handle_demux, "WebPAnimDecoderDelete");

    // DECODE
    FUNC_WebPDecodeRGBAInto = dlsym(handle_webp, "WebPDecodeRGBAInto");

    return JNI_TRUE;
}
JNIEXPORT void JNICALL
Java_com_mycompany_myndkapp_HelloJni_nativeFini(JNIEnv* env, jclass clazz){
	dlclose(handle_demux);
	dlclose(handle_webp);
	dlclose(handle_sharpyuv);
}

JNIEXPORT jlong JNICALL
Java_com_mycompany_myndkapp_HelloJni_nativeOpen(JNIEnv* env, jclass clazz,
                                            jbyteArray bytes)
{
    WebPAnim* anim = calloc(1, sizeof(WebPAnim));
    jsize size = (*env)->GetArrayLength(env, bytes);

    anim->data.bytes = malloc(size);
    anim->data.size = size;

    (*env)->GetByteArrayRegion(env, bytes, 0, size, (jbyte*)anim->data.bytes);

    anim->demux = FUNC_WebPDemuxInternal(&anim->data, 0, NULL, WEBP_DEMUX_ABI_VERSION);
	
    // -------- Prepare demuxer --------
	/*

    WebPAnimDecoderOptions dec_opts;
    FUNC_WebPAnimDecoderOptionsInit(&dec_opts);

    anim->anim_decoder = FUNC_WebPAnimDecoderNew(&anim->data, &dec_opts);

    WebPAnimInfo info;
    if (anim_decoder) {
    	if (!FUNC_WebPAnimDecoderGetInfo(anim_decoder, &info)) {
        	FUNC_WebPAnimDecoderDelete(anim->anim_decoder);
        	free(anim->data.bytes);
			anim->data.bytes = NULL;
    	}
    	anim->canvas_width  = info.canvas_width;
    	anim->canvas_height = info.canvas_height;
    }
	else{
        free(anim->data.bytes);
		anim->data.bytes = NULL;
	}
	*/
    return (jlong)anim;
}

JNIEXPORT jint JNICALL
Java_com_mycompany_myndkapp_HelloJni_nativeGetFrameCount(JNIEnv* env, jclass clazz,
                                                     jlong handle) {
    WebPAnim* a = (WebPAnim*)handle;
    return FUNC_WebPDemuxGetI(a->demux, WEBP_FF_FRAME_COUNT);
}

JNIEXPORT jint JNICALL
Java_com_mycompany_myndkapp_HelloJni_nativeGetFrameDelay(JNIEnv* env, jclass clazz,
                                                     jlong handle, jint frame)
{
    WebPAnim* a = (WebPAnim*)handle;

    WebPIterator iter;
    if (!FUNC_WebPDemuxGetFrame(a->demux, frame, &iter))
        return -1;

    int delay = iter.duration;
    FUNC_WebPDemuxReleaseIterator(&iter);
    return delay;
}

JNIEXPORT jintArray JNICALL
Java_com_mycompany_myndkapp_HelloJni_nativeGetFrameDimension(JNIEnv* env, jclass clazz,
                                                   jlong handle, jint frame)
{
    WebPAnim* a = (WebPAnim*)handle;

    WebPIterator iter;
    if (!FUNC_WebPDemuxGetFrame(a->demux, frame, &iter))
        return NULL;

	jintArray array = (*env)->NewIntArray(env, 2);
	int c_array[2] = {iter.width, iter.height};
	(*env)->SetIntArrayRegion(env, array, 0, 2, c_array);
	
    FUNC_WebPDemuxReleaseIterator(&iter);
	return array;
}

JNIEXPORT jstring JNICALL
Java_com_mycompany_myndkapp_HelloJni_nativeDecodeFrame(JNIEnv* env, jclass clazz,
                                                   jlong handle, jint frame,
                                                   jobject buffer)
{
    WebPAnim* a = (WebPAnim*)handle;

    WebPIterator iter;
    if (!FUNC_WebPDemuxGetFrame(a->demux, frame, &iter))
        return -1;
    int stride = iter.width * 4;

    uint8_t* dst = (*env)->GetDirectBufferAddress(env, buffer);
    size_t capacity = (*env)->GetDirectBufferCapacity(env, buffer);
		
    uint8_t* ptr = FUNC_WebPDecodeRGBAInto(
        iter.fragment.bytes,
        iter.fragment.size,
        dst,
        capacity,
        stride
    );

	char message[4096];
	sprintf(message, "Frame=%d, W=%d, H=%d, Size=%zu, status = %s",
			frame, iter.width, iter.height, capacity, (ptr) ? "SUCCESS" : "FAIL");
    FUNC_WebPDemuxReleaseIterator(&iter);
	
	return (*env)->NewStringUTF(env, message);
}

JNIEXPORT jint JNICALL
Java_com_mycompany_myndkapp_HelloJni_nativeFindFrameAtTime(
        JNIEnv* env, jclass clazz,
        jlong handle, jint t_ms)
{
    WebPAnim* a = (WebPAnim*)handle;

    int frameCount = FUNC_WebPDemuxGetI(a->demux, WEBP_FF_FRAME_COUNT);

    WebPIterator iter;
	int f;
	int endTime = 0;
    for (f = 1; f <= frameCount; f++) {

        if (!FUNC_WebPDemuxGetFrame(a->demux, f, &iter))
            break;

        endTime += iter.duration;

        if (t_ms <= endTime) {
            FUNC_WebPDemuxReleaseIterator(&iter);
            return f; // FOUND FRAME
        }
    }

    // If timestamp exceeds total duration, return last frame
    return frameCount;
}

JNIEXPORT void JNICALL
Java_com_mycompany_myndkapp_HelloJni_nativeClose(JNIEnv* env, jclass clazz,
                                             jlong handle)
{
    WebPAnim* a = (WebPAnim*)handle;
    //FUNC_WebPAnimDecoderDelete(a->anim_decoder);
    FUNC_WebPDemuxDelete(a->demux);
    free((void*)a->data.bytes);
    free(a);
}


/* stringFromJNI */
JNIEXPORT jstring JNICALL Java_com_mycompany_myndkapp_HelloJni_stringFromJNI(JNIEnv* env, jobject thiz)
{
	char text[4096];
	chdir("/sdcard/Android/data/com.mycompany.myndkapp/files");
	mkdir("external_dir", 0666);
	chdir("/data/data/com.mycompany.myndkapp");
	mkdir("internal_dir", 0666);
	system("date > buf.txt");
	// system("env HOME=/data/data/com.mycompany.myndkapp >> buf.txt && ls ${HOME} >> buf.txt");
	system("wc /sdcard/Android/data/com.mycompany.myndkapp/files/ls.txt >> buf.txt");
	system("pwd >> buf.txt && cat buf.txt >> /sdcard/Android/data/com.mycompany.myndkapp/files/ls.txt");
	int fd = open("buf.txt", O_RDONLY);
	if(-1 == fd)
		strcpy(text, "Hello from JNI built on AIDE !\nFile open failed!");
	else
	{
		int br = read(fd, text, sizeof(text) - 1);
		text[br] = '\0';
		close(fd);
	}
	return (*env)->NewStringUTF(env, text);
}

