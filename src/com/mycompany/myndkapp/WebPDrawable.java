package com.mycompany.myndkapp;

import android.content.*;
import android.graphics.*;
import android.graphics.drawable.*;
import android.os.*;
import android.util.*;
import android.widget.*;
import java.nio.*;

public class WebPDrawable extends Drawable implements Runnable
{

    private final Handler handler = new Handler(Looper.getMainLooper());
    private final Paint paint = new Paint(Paint.FILTER_BITMAP_FLAG);
	private int durationMs;

    private final long handle;
	private Context activityContext;

    private final int frameCount;
    private int currentFrame = 1;
    private boolean playing = false;

    private final int[] frameDelay; // ms per frame
    private ByteBuffer decodeBuf = null;
    private Bitmap bitmap = null;

    public WebPDrawable(Context context, byte[] data) {
		this.activityContext = context;
        handle = HelloJni.nativeOpen(data);
        frameCount = HelloJni.nativeGetFrameCount(handle);

        // read each frame delay
		durationMs = 0;
        frameDelay = new int[frameCount + 1];
        for (int i = 1; i <= frameCount; i++) {
            frameDelay[i] = HelloJni.nativeGetFrameDelay(handle, i);
            if (frameDelay[i] < 10) 
				frameDelay[i] = 10;
			durationMs += frameDelay[i];
        }

        decodeFrame(1);
    }

    private void decodeFrame(int f) {
		int[] dimension = HelloJni.nativeGetFrameDimension(handle, f);
		if(null == dimension)
			return;
        bitmap = Bitmap.createBitmap(dimension[0], dimension[1], Bitmap.Config.ARGB_8888);
        decodeBuf = ByteBuffer.allocateDirect(dimension[0] * dimension[1] * 4);
        String decInfoText = HelloJni.nativeDecodeFrame(handle, f, decodeBuf);
		Log.i("DECODE", decInfoText);
		//Toast.makeText(activityContext, decInfoText, Toast.LENGTH_LONG).show();
        decodeBuf.rewind();
        bitmap.copyPixelsFromBuffer(decodeBuf);
    }

    @Override
    public void draw(Canvas canvas) {
        canvas.drawBitmap(bitmap, 0, 0, paint);
    }

    @Override
    public void run() {
        if (!playing) return;

        if (currentFrame > frameCount) currentFrame = 1;

        decodeFrame(currentFrame);
        invalidateSelf();

        handler.postDelayed(this, frameDelay[currentFrame]);
        currentFrame++;
    }

    public void play() {
        if (playing) return;
        playing = true;
        handler.post(this);
    }

    public void pause() {
        playing = false;
    }

    public void seek(int frameIndex) {
        if (frameIndex < 1 || frameIndex > frameCount) return;
        currentFrame = frameIndex;
        decodeFrame(currentFrame);
        invalidateSelf();
    }
	
	public void seekToTimeMs(int t_ms) {
		int f = HelloJni.nativeFindFrameAtTime(handle, t_ms);
		seek(f);
	}
	
	public void dispose(){
		pause();
		HelloJni.nativeClose(handle);
	}
	
	@Override
    public void setAlpha(int alpha) { /* not implemented */ }
    @Override
    public void setColorFilter(ColorFilter colorFilter) { /* not implemented */ }
    @Override
    public int getOpacity() { return android.graphics.PixelFormat.TRANSLUCENT; }
	
	
    public int getFrameCount() { return frameCount; }
    public int getCurrentFrame() { return currentFrame; }
    public int getFrameDelay(int f) { return frameDelay[f]; }
    public int getAnimationDuration() { return durationMs; }
}

