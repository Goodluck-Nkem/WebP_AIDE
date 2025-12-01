package com.mycompany.myndkapp;

import android.graphics.*;
import android.graphics.drawable.*;
import android.net.*;
import android.os.*;
import android.util.*;

public class AVIFDrawable extends Drawable implements Runnable
{
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final Paint paint = new Paint(Paint.FILTER_BITMAP_FLAG);
	private int[] AVIF_frameInfo = new int[]{0, 0, 0, 0}; /* width, height, frameNumber, duration */
	private int[] AVIF_commonInfo = new int[]{0, 0}; /* framecount, total_duration */

    private long handle = 0;
	private HelloJni activityContext;

    private boolean playing = false;

    private Bitmap bitmap = null;

    public AVIFDrawable(HelloJni activityContext, Uri uri)
	{
		this.activityContext = activityContext;
		try{
			ParcelFileDescriptor pfd = activityContext.getContentResolver().openFileDescriptor(uri, "r");
			if (pfd != null && 0 != (handle = HelloJni.avifInit(pfd.getFd())))
			{
				AVIF_commonInfo = HelloJni.avifGetCommonInfo(handle);
				Log.i("AVIF_INFO", getInfoString());
			}
			pfd.close();
		}catch(Exception e){Log.e("AVIF_ERROR", e.toString());}
    }

	public AVIFDrawable validateDrawable()
	{
		return handle == 0 ? null : this;
	}

	public String getInfoString()
	{
		return   
			"\nFrameCount = " + AVIF_commonInfo[HelloJni.AVIF_TOTALCOUNT] + 
			"\nDuration = " + AVIF_commonInfo[HelloJni.AVIF_DURATION] + " ms";
	}

    @Override
    public void draw(Canvas canvas)
	{
        canvas.drawBitmap(bitmap, null, getBounds(), paint);
    }

	@Override
	public int getIntrinsicWidth()
	{
		return bitmap != null ? bitmap.getWidth() : 0;
	}

	@Override
	public int getIntrinsicHeight()
	{
		return bitmap != null ? bitmap.getHeight() : 0;
	}

    @Override
    public void run()
	{
        if (!playing) return;
        seekNext();
        handler.postDelayed(this, AVIF_frameInfo[HelloJni.AVIF_FRAMEDURATION]);
    }

    public void play()
	{
        if (playing) return;
        playing = true;
        handler.post(this);
    }

    public void pause()
	{
        playing = false;
    }

    public void seekNext()
	{
		AVIF_frameInfo = HelloJni.avifDecodeNext(handle);
		bitmap = Bitmap.createBitmap(
			AVIF_frameInfo[HelloJni.AVIF_FRAMEWIDTH], 
			AVIF_frameInfo[HelloJni.AVIF_FRAMEHEIGHT], 
			Bitmap.Config.ARGB_8888);
		HelloJni.avifApplyDecodedPixels(handle, bitmap);
        invalidateSelf();
    }

	public void seekToTimeMs(int t_ms)
	{
		AVIF_frameInfo = HelloJni.avifSeekTo(handle, t_ms);
		bitmap = Bitmap.createBitmap(
			AVIF_frameInfo[HelloJni.AVIF_FRAMEWIDTH], 
			AVIF_frameInfo[HelloJni.AVIF_FRAMEHEIGHT], 
			Bitmap.Config.ARGB_8888);
		HelloJni.avifApplyDecodedPixels(handle, bitmap);
        invalidateSelf();
	}

	public void dispose()
	{
		pause();
		HelloJni.avifFini(handle);
	}

	@Override
    public void setAlpha(int alpha)
	{ /* not implemented */ }
    @Override
    public void setColorFilter(ColorFilter colorFilter)
	{ /* not implemented */ }
    @Override
    public int getOpacity()
	{ return android.graphics.PixelFormat.TRANSLUCENT; }


    public int getFrameCount()
	{ return AVIF_commonInfo[HelloJni.AVIF_TOTALCOUNT]; }
    public int getTotalAnimationDuration()
	{ return AVIF_commonInfo[HelloJni.AVIF_DURATION]; }
	
    public int getCurrentFrame()
	{ return AVIF_frameInfo[HelloJni.AVIF_FRAMENUMBER]; }
    public int getCurrentFrameDuration()
	{ return AVIF_frameInfo[HelloJni.AVIF_FRAMEDURATION]; }
}

