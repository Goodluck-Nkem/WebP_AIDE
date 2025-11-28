#include <jni.h>
#include <dlfcn.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <android/bitmap.h>

#include "webp/demux.h" 
#include "webp/decode.h"

typedef struct {
	WebPData data;
	WebPAnimDecoder* anim_decoder;
	uint8_t* frame_rgba;
	/* keep these 4 sections together */
	uint32_t canvas_width, canvas_height, frame_count, duration_ms;
	int time_stamp, current_frame;
} WebPAnim;

static int get_duration(WebPAnim* a) {
	WebPIterator iter;
	WebPDemuxer* demux;
	a->duration_ms = 0;
	
	// -------- Prepare generic demuxer --------
	if(!(demux = WebPDemux(&a->data)))
		return 0;
	
	if (WebPDemuxGetFrame(demux, 1, &iter)) {
		do {
            if (iter.duration < 10)
				a->duration_ms = 10;
			else
				a->duration_ms += iter.duration;
		} while (WebPDemuxNextFrame(&iter));

		WebPDemuxReleaseIterator(&iter);
	}
	
	WebPDemuxDelete(demux);
	return a->duration_ms;
}

/* init resources */
JNIEXPORT jlong JNICALL
Java_com_mycompany_myndkapp_HelloJni_webpInit(JNIEnv* env, jclass clazz,
		jbyteArray bytes)
{
	WebPAnim* anim = calloc(1, sizeof(WebPAnim));
	jsize size = (*env)->GetArrayLength(env, bytes);

	anim->data.bytes = malloc(size);
	anim->data.size = size;

	/* copy bytes to C memory, then load the duration field */
	(*env)->GetByteArrayRegion(env, bytes, 0, size, (jbyte*)anim->data.bytes);
	if(!get_duration(anim))
		goto webpInitError;

	// -------- Prepare animation demuxer --------
	WebPAnimDecoderOptions dec_opts;
	WebPAnimDecoderOptionsInit(&dec_opts);
	if(!(anim->anim_decoder = WebPAnimDecoderNew(&anim->data, &dec_opts)))
		goto webpInitError;

	/* Get WebP info */
	WebPAnimInfo info;
	if (!WebPAnimDecoderGetInfo(anim->anim_decoder, &info)) {
		WebPAnimDecoderDelete(anim->anim_decoder);
		goto webpInitError;
	}
	
	/* success */
	anim->canvas_width  = info.canvas_width;
	anim->canvas_height = info.canvas_height;
	anim->frame_count = info.frame_count;
	return (jlong)anim;

	/* failure */
webpInitError:
	free(anim->data.bytes);
	anim->data.bytes = NULL;
	free(anim);
	return 0;
}

/* returns width, height, framecount and duration contiguously */
JNIEXPORT jintArray JNICALL
Java_com_mycompany_myndkapp_HelloJni_webpGetInfo(JNIEnv* env, jclass clazz, jlong handle)
{
	WebPAnim* a = (WebPAnim*)handle;
	jintArray infoArray = (*env)->NewIntArray(env, 4);
	/* w, h, fc, d */
	(*env)->SetIntArrayRegion(env, infoArray, 0, 4, &a->canvas_width);
	return infoArray;
}

/* decodes Next Frame, returns its duration */
JNIEXPORT int JNICALL
Java_com_mycompany_myndkapp_HelloJni_webpDecodeNext(JNIEnv* env, jclass clazz, jlong handle, jobject bitmap)
{
	WebPAnim* a = (WebPAnim*)handle;
	
	int previous_ts = a->time_stamp;
	if(WebPAnimDecoderHasMoreFrames(a->anim_decoder) && 
		WebPAnimDecoderGetNext(a->anim_decoder, &a->frame_rgba, &a->time_stamp))
	{
		void* dst;
		AndroidBitmap_lockPixels(env, bitmap, &dst);
		memcpy(dst, a->frame_rgba, (a->canvas_width * a->canvas_height * 4));
		AndroidBitmap_unlockPixels(env, bitmap);
		a->current_frame++;
	}
	
	return (a->time_stamp - previous_ts);
}

/* seeks to the timestamp, returns the frame and duration contiguously */
JNIEXPORT jintArray JNICALL
Java_com_mycompany_myndkapp_HelloJni_webpSeekTo(JNIEnv* env, jclass clazz, jlong handle, jobject bitmap, jint t_ms)
{
	WebPAnim* a = (WebPAnim*)handle;

	/* first reset, then iterate until target frame */
	WebPAnimDecoderReset(a->anim_decoder);
	a->current_frame = 0;
	a->time_stamp = 0;
	int previous_ts = 0;
	while(WebPAnimDecoderHasMoreFrames(a->anim_decoder) && 
		WebPAnimDecoderGetNext(a->anim_decoder, &a->frame_rgba, &a->time_stamp))
	{
		a->current_frame++;
		if(a->time_stamp >= t_ms)
		{
			void* dst;
			AndroidBitmap_lockPixels(env, bitmap, &dst);
			memcpy(dst, a->frame_rgba, (a->canvas_width * a->canvas_height * 4));
			AndroidBitmap_unlockPixels(env, bitmap);
			break;
		}
		previous_ts = a->time_stamp;
	}

	/* return frame info */
	jintArray frameInfo = (*env)->NewIntArray(env, 2);
	int c_array[2] = {a->current_frame, a->time_stamp - previous_ts};
	(*env)->SetIntArrayRegion(env, frameInfo, 0, 2, c_array);
	return frameInfo;
}

/* release resources */
JNIEXPORT void JNICALL
Java_com_mycompany_myndkapp_HelloJni_webpFini(JNIEnv* env, jclass clazz, jlong handle)
{
	WebPAnim* a = (WebPAnim*)handle;
	WebPAnimDecoderDelete(a->anim_decoder);
	free((void*)a->data.bytes);
	a->data.bytes = NULL;
	free(a);
}

