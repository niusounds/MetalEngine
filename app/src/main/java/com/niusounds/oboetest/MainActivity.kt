package com.niusounds.oboetest

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.View
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat

class MainActivity : AppCompatActivity() {
    private val recordPermission = Manifest.permission.RECORD_AUDIO

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Example of a call to a native method
        findViewById<TextView>(R.id.sample_text).text = stringFromJNI()

    }

    fun startAudio(view: View) {
        if (ContextCompat.checkSelfPermission(
                this,
                recordPermission
            ) == PackageManager.PERMISSION_GRANTED
        ) {
            nativeAudio()
        } else {
            requestPermissions(arrayOf(recordPermission), 1)
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        when (requestCode) {
            1 -> {
                permissions.forEachIndexed { index, permission ->
                    if (permission == this.recordPermission && grantResults[index] == PackageManager.PERMISSION_GRANTED) {
                        nativeAudio()
                    }
                }
            }
        }
    }

    override fun onDestroy() {
        release()
        super.onDestroy()
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String
    external fun nativeAudio()
    external fun release()

    companion object {
        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("app")
        }
    }

}