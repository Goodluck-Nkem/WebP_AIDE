/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.mycompany.myndkapp;

import android.app.*;
import android.content.*;
import android.database.*;
import android.net.*;
import android.os.*;
import android.provider.*;
import android.view.*;
import android.widget.*;
import java.io.*;
import java.nio.*;
import java.util.*;


public class HelloJni extends Activity
{
	private static final int REQUEST_OPEN_DOCUMENT = 100;
	private TextView textView;
	private ImageView imageView;
	private Button button, nextFrameBtn;
	private boolean lib_succeed = false;
	private WebPDrawable wd = null;
	private int manual_frame = 0;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        /* Create a TextView and set its content.
         * the text is retrieved by calling a native
         * function.
         */
		setContentView(R.layout.main);
		textView = (TextView)findViewById(R.id.textView);
		button = (Button)findViewById(R.id.button);
		nextFrameBtn = (Button)findViewById(R.id.nextFrameBtn);
		imageView = (ImageView)findViewById(R.id.imageView);
		copy_asset(this, "GIRL1.BMP", "GIRL1.BMP");
		imageView.setImageURI(Uri.fromFile(new File(getFilesDir(), "GIRL1.BMP")));
        textView.setText(stringFromJNI());

		button.setOnClickListener(new View.OnClickListener(){
				@Override
				public void onClick(View v)
				{
					openMediaFile();
				}
			});

		nextFrameBtn.setOnClickListener(new View.OnClickListener(){
				@Override
				public void onClick(View v)
				{
					if(wd == null)
						return;
					wd.pause();
					int totalFrames = wd.getFrameCount();
					wd.seek(++manual_frame);
					if(manual_frame >= totalFrames)
						manual_frame = 1;
				}
			});
		
		copy_asset(this, "libsharpyuv.so", "libsharpyuv.so");
		copy_asset(this, "libwebp.so", "libwebp.so");
		copy_asset(this, "libwebpdemux.so", "libwebpdemux.so");
		lib_succeed = nativeInit(getFilesDir().getAbsolutePath() + "/libsharpyuv.so", 
										 getFilesDir().getAbsolutePath() + "/libwebp.so",
										 getFilesDir().getAbsolutePath() + "/libwebpdemux.so");
		Toast.makeText(this, lib_succeed ? "nativeInit success" : "nativeInit failure", Toast.LENGTH_SHORT).show();
    }

	/* fun fact: this AIDE is sdk 24 */
	private void openMediaFile()
	{
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE).setType("*/*");
		intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"image/*", "audio/*", "video/*", "application/octet-stream"});
        startActivityForResult(intent, REQUEST_OPEN_DOCUMENT);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == REQUEST_OPEN_DOCUMENT && resultCode == RESULT_OK)
		{
            if (data != null)
			{
				if(wd != null)
					wd.dispose();
                Uri uri = data.getData();
				String displayName = getContentDisplayName(this, uri);
				String imageInfoMsg = displayName;
				if (lib_succeed && displayName.endsWith(".webp"))
				{
					try
					{
						byte[] br = readBytes(this, uri);
						wd = new WebPDrawable(this, br);
						imageInfoMsg += "\nDuration_MS: " + wd.getAnimationDuration() + "\nMiB: " + (float)br.length / (1024f * 1024f) +"\nFrameCount: " + wd.getFrameCount();
						imageView.setImageDrawable(wd);
						//wd.play();
					}
					catch (IOException e)
					{}
				}
				else
					imageView.setImageURI(uri);
				textView.setText(imageInfoMsg);
            }
        }
	}

	public static byte[] readBytes(Context context, Uri uri) throws IOException
	{
		InputStream inputStream = context.getContentResolver().openInputStream(uri);
		if (inputStream == null) return null;

		ByteArrayOutputStream buffer = new ByteArrayOutputStream();

		byte[] data = new byte[8192];
		int n;
		while ((n = inputStream.read(data)) != -1)
		{
			buffer.write(data, 0, n);
		}

		inputStream.close();
		return buffer.toByteArray();
	}

	public static String getContentDisplayName(Context context, Uri uri)
	{
        String displayName = "";
        try
		{
			Cursor cursor = context.getContentResolver().query(uri, null, null, null, null);
            if (cursor != null && cursor.moveToFirst())
			{
                int index = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (index != -1)
                    displayName = cursor.getString(index).toLowerCase();
				cursor.close();
            }
        }
		catch (Exception e)
		{
            return "";
        }
        return displayName;
    }

    public static void copy_asset(Context context, String asset_name, String output_name)
	{
        try
		{
			InputStream in = context.getAssets().open(asset_name);
			OutputStream out = new FileOutputStream(new File(context.getFilesDir(), output_name));
            int br;
            byte[] buffer = new byte[4096];
            while (-1 != (br = in.read(buffer)))
			{
                out.write(buffer, 0, br);
            }
            out.flush();
			in.close();
			out.close();
        }
		catch (IOException e)
		{
            Toast.makeText(context, "Asset " + asset_name + " Not Found!" + e, Toast.LENGTH_SHORT).show();
        }
    }

	@Override
	protected void onDestroy()
	{
		super.onDestroy();
		if(wd != null)
			wd.dispose();
		wd = null;
		nativeFini();
	}
	

    public native String stringFromJNI();
	public static native boolean nativeInit(String sharpyuv, String webp, String webpdemux);
	public static native void nativeFini();
	public static native long nativeOpen(byte[] bytes); 
	public static native int nativeGetFrameCount(long handle);
	public static native int nativeGetFrameDelay(long handle, int frame);
	public static native int[] nativeGetFrameDimension(long handle, int frame); 
	public static native String nativeDecodeFrame(long handle, int frame, ByteBuffer buffer);
	public static native int nativeFindFrameAtTime(long handle, int t_ms); 
	public static native void nativeClose(long handle);


    /* this is used to load the 'hello-jni' library on application
     * startup. The library has already been unpacked into
     * /data/data/com.example.hellojni/lib/libhello-jni.so at
     * installation time by the package manager.
     */
    static {
        System.loadLibrary("hello-jni");
    }
}
