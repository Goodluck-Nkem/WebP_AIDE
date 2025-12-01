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
import android.graphics.*;
import android.net.*;
import android.os.*;
import android.provider.*;
import android.view.*;
import android.view.inputmethod.*;
import android.widget.*;
import java.io.*;


public class HelloJni extends Activity
{
	private static final int REQUEST_OPEN_DOCUMENT = 100;
	public ImageView imageView;
	public TextView textView;
	private Button selectFileBtn, nextFrameBtn, playPauseBtn;
	private EditText seekEdit;
	private WebPDrawable webpDrawable = null;
	private AVIFDrawable avifDrawable = null;
	private String currentMediaDisplayName = "";
	
	public String getCurrentMediaDisplayName(){return currentMediaDisplayName;}

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

		setContentView(R.layout.main);
		textView = (TextView)findViewById(R.id.textView);
		selectFileBtn = (Button)findViewById(R.id.selectFileBtn);
		nextFrameBtn = (Button)findViewById(R.id.nextFrameBtn);
		playPauseBtn = (Button)findViewById(R.id.playPauseBtn);
		seekEdit = (EditText)findViewById(R.id.seekEdit);
		imageView = (ImageView)findViewById(R.id.imageView);
		copy_asset(this, "GIRL1.BMP", "GIRL1.BMP");
		imageView.setImageURI(Uri.fromFile(new File(getFilesDir(), "GIRL1.BMP")));
        textView.setText("GIRL1 (BMP)");

		selectFileBtn.setOnClickListener(new View.OnClickListener(){
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
					if(webpDrawable != null)webpDrawable.seekNext();
					if(avifDrawable != null)avifDrawable.seekNext();
				}
			});
			
		playPauseBtn.setOnClickListener(new View.OnClickListener(){
				@Override
				public void onClick(View p1)
				{
					if(webpDrawable != null)
					{
						if(webpDrawable.isPlaying())
							webpDrawable.pause();
						else
							webpDrawable.play();
					}
					if(avifDrawable != null)
					{
						if(avifDrawable.isPlaying())
							avifDrawable.pause();
						else
							avifDrawable.play();
					}
				}
			});
			
		seekEdit.setOnEditorActionListener(new EditText.OnEditorActionListener(){
				@Override
				public boolean onEditorAction(TextView tv, int actionId, KeyEvent event)
				{
					if (actionId == EditorInfo.IME_ACTION_DONE ||
						(event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER && 
						event.getAction() == KeyEvent.ACTION_DOWN)) 
					{
						// Perform action
						if(webpDrawable != null)
							webpDrawable.seekToTimeMs(Integer.parseInt(tv.getText().toString()));
						if(avifDrawable != null)
							avifDrawable.seekToTimeMs(Integer.parseInt(tv.getText().toString()));

						// afterwards, hide the soft keyboard
						InputMethodManager imm = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
						imm.hideSoftInputFromWindow(tv.getWindowToken(), 0);

						return true; // event handled
					}
					return false; // otherwise let default behavior happen
				}
			});
		
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
                Uri uri = data.getData();
				currentMediaDisplayName = getContentDisplayName(this, uri);
				String imageInfoMsg = "";
				
				if(webpDrawable != null)
				{
					webpDrawable.dispose();
					webpDrawable = null;
				}
				if(avifDrawable != null)
				{
					avifDrawable.dispose();
					avifDrawable = null;
				}
				
				if (currentMediaDisplayName.endsWith(".webp"))
				{
					if((webpDrawable = new WebPDrawable(this, uri).validateDrawable()) != null)
					{
						imageInfoMsg += webpDrawable.getInfoString();
						webpDrawable.play();
						imageView.setImageDrawable(webpDrawable);
					}else{
						imageInfoMsg += "\nError:: Could not Access WebP";
					}
				}
				else if (currentMediaDisplayName.endsWith(".avif"))
				{
					if((avifDrawable = new AVIFDrawable(this, uri).validateDrawable()) != null)
					{
						imageInfoMsg += avifDrawable.getInfoString();
						avifDrawable.play();
						imageView.setImageDrawable(avifDrawable);
					}else{
						imageInfoMsg += "\nError:: Could not Access AVIF";
					}
				}
				else
				{
					imageInfoMsg += currentMediaDisplayName;
					imageView.setImageURI(uri);
				}
				textView.setText(imageInfoMsg);
            }
        }
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
		if(webpDrawable != null)
		{
			webpDrawable.dispose();
			webpDrawable = null;
		}
		if(avifDrawable != null)
		{
			avifDrawable.dispose();
			avifDrawable = null;
		}
		super.onDestroy();
	}

	/* w, h, fc, d */
	public static final int WEBP_WIDTH =  0;
	public static final int WEBP_HEIGHT = 1;
	public static final int WEBP_FRAMECOUNT = 2;
	public static final int WEBP_DURATION = 3;
	
	public static final int AVIF_TOTALCOUNT =  0;
	public static final int AVIF_DURATION = 1;
	
	public static final int AVIF_FRAMEWIDTH =  0;
	public static final int AVIF_FRAMEHEIGHT = 1;
	public static final int AVIF_FRAMENUMBER = 2;
	public static final int AVIF_FRAMEDURATION = 3;
	
	/* WebP native methods */
	public static native long webpInit(int fd);
	public static native int[] webpGetInfo(long handle); /* w, h, fc, d */
	public static native int webpDecodeNext(long handle, Bitmap bitmap);
	public static native int[] webpSeekTo(long handle, Bitmap bitmap, int t_ms); 
	public static native void webpFini(long handle);
	
	/* AVIF native methods */
	public static native long avifInit(int fd);
	public static native int[] avifGetCommonInfo(long handle); /* fc, d */
	public static native int[] avifDecodeNext(long handle); /* w, h, fn, d */
	public static native int[] avifSeekTo(long handle, int t_ms); /* w, h, fn, d */
	public static native void avifApplyDecodedPixels(long handle, Bitmap bitmap); 
	public static native void avifFini(long handle);

    static {
        System.loadLibrary("hello-jni");
    }
}

