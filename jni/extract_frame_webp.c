#include <stdio.h>
#include <stdlib.h>
#include <webp/decode.h>
#include <webp/demux.h>
#include <webp/encode.h>

int main() {
    const char* in_file  = "input.webp";
    const char* out_file = "frame_43.webp";  // output single-frame
    int target_frame = 43;                   // 0-based frame index

    // -------- Read WebP file --------
    FILE* f = fopen(in_file, "rb");
    if (!f) { printf("Cannot open input\n"); return 1; }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    // -------- Prepare demuxer --------
    WebPData webp_data;
    webp_data.bytes = data;
    webp_data.size  = size;

    WebPAnimDecoderOptions dec_opts;
    WebPAnimDecoderOptionsInit(&dec_opts);

    WebPAnimDecoder* dec = WebPAnimDecoderNew(&webp_data, &dec_opts);
    if (!dec) {
        printf("Decoder init failed\n");
        free(data);
        return 1;
    }

    WebPAnimInfo info;
    if (!WebPAnimDecoderGetInfo(dec, &info)) {
        printf("Anim info failed\n");
        WebPAnimDecoderDelete(dec);
        free(data);
        return 1;
    }

    int width  = info.canvas_width;
    int height = info.canvas_height;

    // -------- Iterate until desired frame --------
    uint8_t* frame_rgba = NULL;
    int timestamp = 0;
    int frame_idx = 0;

    while (WebPAnimDecoderHasMoreFrames(dec)) {

        if (!WebPAnimDecoderGetNext(dec, &frame_rgba, &timestamp)) {
            printf("Decode error\n");
            break;
        }

        if (frame_idx == target_frame) {

            // ---- Encode this frame as WebP ----
            uint8_t* out_buf = NULL;
            size_t out_size = WebPEncodeLosslessRGBA(frame_rgba,
                                                     width, height,
                                                     width * 4,
                                                     &out_buf);
            if (out_size == 0) {
                printf("WebP encode failed\n");
            } else {
                // write output file
                FILE* fo = fopen(out_file, "wb");
                fwrite(out_buf, 1, out_size, fo);
                fclose(fo);
                printf("Saved: %s (%zu bytes)\n", out_file, out_size);
            }

            WebPFree(out_buf);
            break;
        }

        frame_idx++;
    }

    WebPAnimDecoderDelete(dec);
    free(data);
    return 0;
}


