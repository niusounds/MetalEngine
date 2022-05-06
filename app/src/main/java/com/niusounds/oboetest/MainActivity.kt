package com.niusounds.oboetest

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.niusounds.metalengine.AudioOutput

class MainActivity : AppCompatActivity() {
    private val recordPermission = Manifest.permission.RECORD_AUDIO
    private val toneGenerator: ToneGenerator by lazy {
        ToneGenerator(
            engine = AudioOutput.create(
                sampleRate = 48000,
                frameSize = 1920,
                channels = 1,
                bufferCount = 16,
            )
        )
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
    }

    fun startAudio(view: View) {
        if (ContextCompat.checkSelfPermission(
                this,
                recordPermission
            ) == PackageManager.PERMISSION_GRANTED
        ) {
            toneGenerator.start()
        } else {
            requestPermissions(arrayOf(recordPermission), 1)
        }
    }

    override fun onStop() {
        super.onStop()
        toneGenerator.stop()
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
                        toneGenerator.start()
                    }
                }
            }
        }
    }

    override fun onDestroy() {
        toneGenerator.release()
        super.onDestroy()
    }
}