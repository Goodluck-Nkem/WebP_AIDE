#include <jni.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <android/bitmap.h>

#include "avif/avif.h"

typedef struct {
	avifDecoder *dec;
	avifRGBImage* rgb;
	uint8_t* inputBytes;
	double total_duration_sec;
	double* frame_duration_sec;
	int currentFrameNumber;
} AVIFAnim;

// Load file into memory provided an FD
static uint8_t *read_fd(int fd, size_t *file_size)
{
	/* assume the FD is already rewinded */
    struct stat fd_stat;
	fstat(fd, &fd_stat);
	*file_size = fd_stat.st_size;
    uint8_t *data = malloc(*file_size);
    read(fd, data, *file_size);
    return data; 
	/* assume the FD provider will close the FD by itself when desired */
}

/* init resources, returns handle */
JNIEXPORT jlong JNICALL
Java_com_mycompany_myndkapp_HelloJni_avifInit(JNIEnv* env, jclass clazz, jint fd)
{
	/* allocate handle and zero memory */
	AVIFAnim* anim = calloc(1, sizeof(AVIFAnim));

	/* copy the file bytes */
	size_t file_size;
	anim->inputBytes = read_fd(fd, &file_size);

	// -------- Prepare AVIF decoder --------
	anim->dec = avifDecoderCreate();
    if (!anim->dec) 
		goto avifErrorCreateDec;

    if (AVIF_RESULT_OK != avifDecoderSetIOMemory(anim->dec, anim->inputBytes, file_size)
		|| AVIF_RESULT_OK != avifDecoderParse(anim->dec))
		goto avifErrorParseDec;

	/* get all duration for the Animation */
    anim->frame_duration_sec = malloc(sizeof(double) * anim->dec->imageCount);
    anim->total_duration_sec = 0.0;
    int index = 0;
	avifResult res;
    while ((res = avifDecoderNextImage(anim->dec)) == AVIF_RESULT_OK)
    {
        anim->frame_duration_sec[index++] = anim->dec->imageTiming.duration;
        anim->total_duration_sec += anim->dec->imageTiming.duration;
    }
	
	/* success */
    avifDecoderReset(anim->dec);
	return (jlong)anim;

	/* failure */
avifErrorParseDec:
    avifDecoderDestroy(anim->dec);
avifErrorCreateDec:
	free(anim->inputBytes);
	anim->inputBytes = NULL;
	free(anim);
	return 0;
}

/* returns anim info which is "framecount" and "total_duration" contiguously */
JNIEXPORT jintArray JNICALL
Java_com_mycompany_myndkapp_HelloJni_avifGetCommonInfo(JNIEnv* env, jclass clazz, jlong handle)
{
	AVIFAnim* a = (AVIFAnim*)handle;
	jintArray infoArray = (*env)->NewIntArray(env, 2); /* fc, d */
	int c_array[2] = {a->dec->imageCount, (int)(a->total_duration_sec * 1000)};
	(*env)->SetIntArrayRegion(env, infoArray, 0, 2, c_array);
	return infoArray;
}

/* decodes Next Frame, returns its frame info which is "width", "height", "frame_number" and "duration" contiguously */
JNIEXPORT jintArray JNICALL
Java_com_mycompany_myndkapp_HelloJni_avifDecodeNext(JNIEnv* env, jclass clazz, jlong handle)
{
	AVIFAnim* a = (AVIFAnim*)handle;
	
	if(!a->rgb)
		a->rgb = malloc(sizeof(avifRGBImage));
	else
		avifRGBImageFreePixels(a->rgb);
	
	if(a->currentFrameNumber < a->dec->imageCount)
	{
    	avifDecoderNextImage(a->dec);
		a->currentFrameNumber++;
	}
	
	/* get the RGBA pixels */
    avifRGBImageSetDefaults(a->rgb, a->dec->image);
    a->rgb->format = AVIF_RGB_FORMAT_RGBA;
	avifRGBImageAllocatePixels(a->rgb);
	avifImageYUVToRGB(a->dec->image, a->rgb);
	
	/* return frame info */
	jintArray frameInfo = (*env)->NewIntArray(env, 4); /* w, h, fn, d */
	int c_array[4] = {a->rgb->width, a->rgb->height, a->currentFrameNumber, (int)(a->frame_duration_sec[a->currentFrameNumber - 1] * 1000)};  
	(*env)->SetIntArrayRegion(env, frameInfo, 0, 4, c_array);
	return frameInfo;
}


/* seeks to the timestamp, returns its frame info which is "width", "height", "frame_number" and "duration" contiguously */
JNIEXPORT jintArray JNICALL
Java_com_mycompany_myndkapp_HelloJni_avifSeekTo(JNIEnv* env, jclass clazz, jlong handle, jint t_ms)
{
	AVIFAnim* a = (AVIFAnim*)handle;

	if(!a->rgb)
		a->rgb = malloc(sizeof(avifRGBImage));
	else
		avifRGBImageFreePixels(a->rgb);
	
    double queryTime = ((double)t_ms) / 1000.0;
    double sum = 0;
    int targetFrameIndex = a->dec->imageCount - 1; /* last frame is the default  */

	int i;
    for (i = 0; i < a->dec->imageCount; i++) {
        sum += a->frame_duration_sec[i];
        if (sum >= queryTime) {
            targetFrameIndex = i;
            break;
        }
    }

    // Decode target frame
    avifDecoderReset(a->dec);
    avifDecoderNthImage(a->dec, targetFrameIndex);
	
	/* get the RGBA pixels */
    avifRGBImageSetDefaults(a->rgb, a->dec->image);
    a->rgb->format = AVIF_RGB_FORMAT_RGBA;
	avifRGBImageAllocatePixels(a->rgb);
	avifImageYUVToRGB(a->dec->image, a->rgb);

	/* return frame info */
	jintArray frameInfo = (*env)->NewIntArray(env, 4);
	int c_array[4] = {a->rgb->width, a->rgb->height, targetFrameIndex + 1, (int)(a->frame_duration_sec[i] * 1000)};  
	(*env)->SetIntArrayRegion(env, frameInfo, 0, 4, c_array);
	return frameInfo;
}

/* apply the recently decoded pixels to a bitmap */
JNIEXPORT void JNICALL
Java_com_mycompany_myndkapp_HelloJni_avifApplyDecodedPixels(JNIEnv* env, jclass clazz, jlong handle, jobject bitmap)
{
	AVIFAnim* a = (AVIFAnim*)handle;
	/* copy pixels to bitmap */
	void* dst;
	AndroidBitmap_lockPixels(env, bitmap, &dst);
	memcpy(dst, a->rgb->pixels, (a->rgb->width * a->rgb->height * 4));
	AndroidBitmap_unlockPixels(env, bitmap);
}

/* release resources */
JNIEXPORT void JNICALL
Java_com_mycompany_myndkapp_HelloJni_avifFini(JNIEnv* env, jclass clazz, jlong handle)
{
	AVIFAnim* a = (AVIFAnim*)handle;
	if(a->rgb)
	{
		avifRGBImageFreePixels(a->rgb);
		free(a->rgb);
		a->rgb = NULL;
	}
    avifDecoderDestroy(a->dec);
	free(a->frame_duration_sec);
	free(a->inputBytes);
	a->frame_duration_sec;
	a->inputBytes = NULL;
	free(a);
}


