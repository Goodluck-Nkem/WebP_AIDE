#include <stdio.h>
#include <stdlib.h>
#include "avif/avif.h"

// Load file into memory
static uint8_t *readFile(const char *path, size_t *fileSize)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    *fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *data = malloc(*fileSize);
    fread(data, 1, *fileSize, f);
    fclose(f);
    return data;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s anim.avif\n", argv[0]);
        return 1;
    }

    size_t fileSize;
    uint8_t *fileData = readFile(argv[1], &fileSize);
    if (!fileData) {
        printf("Failed to read file\n");
        return 1;
    }

    avifDecoder *dec = avifDecoderCreate();
    if (!dec) {
        printf("Failed to create decoder\n");
        return 1;
    }

    // Correct way for your libavif version
    (void)avifDecoderSetIOMemory(dec, fileData, fileSize);

    avifResult res = avifDecoderParse(dec);
    if (res != AVIF_RESULT_OK) {
        printf("Parse failed: %s\n", avifResultToString(res));
        goto cleanup;
    }

    printf("Frame count = %d\n", dec->imageCount);

    double *frameDurations = malloc(sizeof(double) * dec->imageCount);

    double totalDuration = 0.0;
    int index = 0;
    while ((res = avifDecoderNextImage(dec)) == AVIF_RESULT_OK)
    {
		//printf("\x1B[33mFrame %d, tsc %lu, pts %g, pts_tsc %lu, dur %g, dur_tsc %lu\x1B[0m\n", index, dec->imageTiming.timescale, dec->imageTiming.pts, dec->imageTiming.ptsInTimescales, dec->imageTiming.duration, dec->imageTiming.durationInTimescales);
		printf("frameIndex %3d, duration %g\n", index, dec->imageTiming.duration);

        frameDurations[index] = dec->imageTiming.duration;
        totalDuration += dec->imageTiming.duration;
        index++;
    }

    printf("Total animation duration = %.3f sec\n", totalDuration);

    // Pick frame for timestamp
    double queryTime = 1.5;
    double sum = 0;
    int targetFrame = 0;

    for (int i = 0; i < dec->imageCount; i++) {
        sum += frameDurations[i];
        if (queryTime < sum) {
            targetFrame = i;
            break;
        }
    }

    printf("\nTimestamp %.3f → frameIndex %d\n", queryTime, targetFrame);

    // Decode target frame
    (void)avifDecoderReset(dec);
    (void)avifDecoderNthImage(dec, targetFrame);

    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, dec->image);
    rgb.format = AVIF_RGB_FORMAT_RGBA;

    (void)avifRGBImageAllocatePixels(&rgb);

    res = avifImageYUVToRGB(dec->image, &rgb);
    if (res != AVIF_RESULT_OK) {
        printf("YUV→RGB failed: %s\n", avifResultToString(res));
        goto cleanup_rgb;
    }

    printf("Decoded frameIndex %d: %dx%d\n",
           targetFrame, rgb.width, rgb.height);

    printf("Pixel[0] = %d %d %d %d\n",
           rgb.pixels[0], rgb.pixels[1],
           rgb.pixels[2], rgb.pixels[3]);

cleanup_rgb:
    avifRGBImageFreePixels(&rgb);

cleanup:
    avifDecoderDestroy(dec);
    free(frameDurations);
    free(fileData);
    return 0;
}

