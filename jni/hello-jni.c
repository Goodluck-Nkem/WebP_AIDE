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

JNIEXPORT jlong JNICALL
Java_com_mycompany_myndkapp_HelloJni_nativeOpen(JNIEnv* env, jclass clazz,
                                            jbyteArray bytes)
{
    WebPAnim* anim = calloc(1, sizeof(WebPAnim));
    jsize size = (*env)->GetArrayLength(env, bytes);

    anim->data.bytes = malloc(size);
    anim->data.size = size;

    (*env)->GetByteArrayRegion(env, bytes, 0, size, (jbyte*)anim->data.bytes);

    anim->demux = WebPDemuxInternal(&anim->data, 0, NULL, WEBP_DEMUX_ABI_VERSION);
	
    // -------- Prepare demuxer --------
	/*

    WebPAnimDecoderOptions dec_opts;
    WebPAnimDecoderOptionsInit(&dec_opts);

    anim->anim_decoder = WebPAnimDecoderNew(&anim->data, &dec_opts);

    WebPAnimInfo info;
    if (anim_decoder) {
    	if (!WebPAnimDecoderGetInfo(anim_decoder, &info)) {
        	WebPAnimDecoderDelete(anim->anim_decoder);
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
    return WebPDemuxGetI(a->demux, WEBP_FF_FRAME_COUNT);
}

JNIEXPORT jint JNICALL
Java_com_mycompany_myndkapp_HelloJni_nativeGetFrameDelay(JNIEnv* env, jclass clazz,
                                                     jlong handle, jint frame)
{
    WebPAnim* a = (WebPAnim*)handle;

    WebPIterator iter;
    if (!WebPDemuxGetFrame(a->demux, frame, &iter))
        return -1;

    int delay = iter.duration;
    WebPDemuxReleaseIterator(&iter);
    return delay;
}

JNIEXPORT jintArray JNICALL
Java_com_mycompany_myndkapp_HelloJni_nativeGetFrameDimension(JNIEnv* env, jclass clazz,
                                                   jlong handle, jint frame)
{
    WebPAnim* a = (WebPAnim*)handle;

    WebPIterator iter;
    if (!WebPDemuxGetFrame(a->demux, frame, &iter))
        return NULL;

	jintArray array = (*env)->NewIntArray(env, 2);
	int c_array[2] = {iter.width, iter.height};
	(*env)->SetIntArrayRegion(env, array, 0, 2, c_array);
	
    WebPDemuxReleaseIterator(&iter);
	return array;
}

JNIEXPORT jstring JNICALL
Java_com_mycompany_myndkapp_HelloJni_nativeDecodeFrame(JNIEnv* env, jclass clazz,
                                                   jlong handle, jint frame,
                                                   jobject buffer)
{
    WebPAnim* a = (WebPAnim*)handle;

    WebPIterator iter;
    if (!WebPDemuxGetFrame(a->demux, frame, &iter))
        return -1;
    int stride = iter.width * 4;

    uint8_t* dst = (*env)->GetDirectBufferAddress(env, buffer);
    size_t capacity = (*env)->GetDirectBufferCapacity(env, buffer);
		
    uint8_t* ptr = WebPDecodeRGBAInto(
        iter.fragment.bytes,
        iter.fragment.size,
        dst,
        capacity,
        stride
    );

	char message[4096];
	sprintf(message, "Frame=%d, W=%d, H=%d, Size=%zu, status = %s",
			frame, iter.width, iter.height, capacity, (ptr) ? "SUCCESS" : "FAIL");
    WebPDemuxReleaseIterator(&iter);
	
	return (*env)->NewStringUTF(env, message);
}

JNIEXPORT jint JNICALL
Java_com_mycompany_myndkapp_HelloJni_nativeFindFrameAtTime(
        JNIEnv* env, jclass clazz,
        jlong handle, jint t_ms)
{
    WebPAnim* a = (WebPAnim*)handle;

    int frameCount = WebPDemuxGetI(a->demux, WEBP_FF_FRAME_COUNT);

    WebPIterator iter;
	int f;
	int endTime = 0;
    for (f = 1; f <= frameCount; f++) {

        if (!WebPDemuxGetFrame(a->demux, f, &iter))
            break;

        endTime += iter.duration;

        if (t_ms <= endTime) {
            WebPDemuxReleaseIterator(&iter);
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
    //WebPAnimDecoderDelete(a->anim_decoder);
    WebPDemuxDelete(a->demux);
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

