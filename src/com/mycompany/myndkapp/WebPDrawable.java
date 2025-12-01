package com.mycompany.myndkapp;

import android.graphics.*;
import android.graphics.drawable.*;
import android.net.*;
import android.os.*;
import android.util.*;

public class WebPDrawable extends Drawable implements Runnable
{
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final Paint paint = new Paint(Paint.FILTER_BITMAP_FLAG);
	private int frameDuration = 10;
	private int[] WebpInfo = new int[]{0, 0, 0, 0};

    private long handle = 0;
	private HelloJni activityContext;

    private int currentFrame = 1;
    private boolean playing = false;

    private Bitmap bitmap = null;

    public WebPDrawable(HelloJni activityContext, Uri uri)
	{
		this.activityContext = activityContext;
		try{
			ParcelFileDescriptor pfd = activityContext.getContentResolver().openFileDescriptor(uri, "r");
			if (pfd != null && 0 != (handle = HelloJni.webpInit(pfd.getFd())))
			{
				WebpInfo = HelloJni.webpGetInfo(handle);
				bitmap = Bitmap.createBitmap(
					WebpInfo[HelloJni.WEBP_WIDTH], WebpInfo[HelloJni.WEBP_HEIGHT], Bitmap.Config.ARGB_8888);
				Log.i("WEBP_INFO", getInfoString());
			}
			pfd.close();
		}catch(Exception e){Log.e("WEBP_ERROR", e.toString());}
    }

	public WebPDrawable validateDrawable()
	{
		return handle == 0 ? null : this;
	}
	
	public String getInfoString()
	{
		return  
			activityContext.getCurrentMediaDisplayName() +
			"\nWidth = " + WebpInfo[HelloJni.WEBP_WIDTH] + 
			", Height = " + WebpInfo[HelloJni.WEBP_HEIGHT] + 
			"\nFrameCount = " + WebpInfo[HelloJni.WEBP_FRAMECOUNT] + 
			"\nDuration = " + WebpInfo[HelloJni.WEBP_DURATION] + " ms";
	}

    @Override
    public void draw(Canvas canvas)
	{
        canvas.drawBitmap(bitmap, null, getBounds(), paint);
		activityContext.textView.setText(getInfoString() + "\nFrame: " + currentFrame); 
    }

	@Override
	public int getIntrinsicWidth()
	{
		return bitmap.getWidth();
	}

	@Override
	public int getIntrinsicHeight()
	{
		return bitmap.getHeight();
	}
	
    @Override
    public void run()
	{
        if (!playing) return;
        seekNext();
        handler.postDelayed(this, frameDuration);
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
	
	public boolean isPlaying()
	{
		return playing;
	}

    public void seekNext()
	{
		frameDuration = HelloJni.webpDecodeNext(handle, bitmap);
        if (currentFrame < WebpInfo[HelloJni.WEBP_FRAMECOUNT]) 
			currentFrame++;
        invalidateSelf();
    }

	public void seekToTimeMs(int t_ms)
	{
		int[] seekDetails = HelloJni.webpSeekTo(handle, bitmap, t_ms);
		currentFrame = seekDetails[0];
		frameDuration = seekDetails[1];
        invalidateSelf();
	}

	public void dispose()
	{
		pause();
		HelloJni.webpFini(handle);
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
	{ return WebpInfo[HelloJni.WEBP_FRAMECOUNT]; }
    public int getTotalAnimationDuration()
	{ return WebpInfo[HelloJni.WEBP_DURATION]; }
	
    public int getCurrentFrame()
	{ return currentFrame; }
    public int getCurrentFrameDuration()
	{ return frameDuration; }
}

