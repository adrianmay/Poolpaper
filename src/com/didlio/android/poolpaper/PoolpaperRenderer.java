package com.didlio.android.poolpaper;
      
import javax.microedition.khronos.egl.EGLConfig;              
import javax.microedition.khronos.opengles.GL10; 
     
import android.content.Context;  
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.BitmapFactory; 
import android.graphics.Matrix;    
import android.opengl.GLSurfaceView;     
import android.opengl.GLUtils;                        
 
import com.didlio.android.poolpaper.*;        
 
                                                   
public class PoolpaperRenderer implements GLSurfaceView.Renderer {
                               
	private PoolpaperService service;
	public PoolpaperRenderer(PoolpaperService s) {service=s;}
    public void onDrawFrame(GL10 gl) {                    
        C.step();          
    }                                

    public void onSurfaceChanged(GL10 gl, int width, int height) {
        C.init(width, height);   
    }                        
        
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {                                                                  
        C.bitmap(loadTexture(gl, service, R.drawable.beige_tiles));               
    }   
	      
 // Get a new texture id:   
    private static int newTextureID(GL10 gl) {  
        int[] temp = new int[1];
        gl.glGenTextures(1, temp, 0);
        return temp[0];        
    }

    // Will load a texture out of a drawable resource file, and return an OpenGL texture ID:
    private int loadTexture(GL10 gl, Context context, int resource) {
        
        // In which ID will we be storing this texture?
        int id = newTextureID(gl);       
        
        // We need to flip the textures vertically:
        Matrix flip = new Matrix();
        flip.postScale(1f, -1f);
         
        // This will tell the BitmapFactory to not scale based on the device's pixel density:
        // (Thanks to Matthew Marshall for this bit)
        BitmapFactory.Options opts = new BitmapFactory.Options();
        opts.inScaled = false;    
                       
        
        // Load up, and flip the texture:
        Bitmap temp = BitmapFactory.decodeResource(context.getResources(), resource, opts);
        Bitmap bmp = Bitmap.createBitmap(temp, 0, 0, temp.getWidth(), temp.getHeight(), flip, true);
        temp.recycle();     
                                                            
        gl.glBindTexture(GL10.GL_TEXTURE_2D, id);    
                                                                    
                         
        // Set all of our texture parameters:
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR_MIPMAP_NEAREST);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_NEAREST);              
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_S, GL10.GL_REPEAT);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_T, GL10.GL_REPEAT);
        
        mipMap(gl, bmp, GL10.GL_TEXTURE_2D);
        
        return id;
    }

    private void mipMap(GL10 gl, Bitmap bmp, int role)
    {
        // Generate, and load up all of the mipmaps:
        for(int level=0, height = bmp.getHeight(), width = bmp.getWidth(); true; level++) {
            // Push the bitmap onto the GPU:
            GLUtils.texImage2D(role, level, bmp, 0);
            
            // We need to stop when the texture is 1x1:   
            if(height==1 && width==1) break;
            
            // Resize, and let's go again:  
            width >>= 1; height >>= 1;
            if(width<1)  width = 1;
            if(height<1) height = 1;
            
            Bitmap bmp2 = Bitmap.createScaledBitmap(bmp, width, height, true);
            //if (level>5)    
            // bmp2.eraseColor(Color.RED);
            bmp.recycle();
            bmp = bmp2;
        }             
        bmp.recycle();
    }
}
