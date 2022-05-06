package com.niusounds.oboetest

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.niusounds.metalengine.AudioInput
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
    private val audioFeedback: AudioFeedback by lazy {
        AudioFeedback(
            input = AudioInput.create(
                sampleRate = 48000,
                frameSize = 1920,
                channels = 1,
            ),
            output = AudioOutput.create(
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
            when (view.id) {
                R.id.toneGenerator -> {
                    toneGenerator.start()
                }
                R.id.feedback -> {
                    audioFeedback.start()
                }
            }
        } else {
            requestPermissions(arrayOf(recordPermission), 1)
        }
    }

    override fun onStop() {
        super.onStop()
        toneGenerator.stop()
        audioFeedback.stop()
    }

    override fun onDestroy() {
        toneGenerator.release()
        audioFeedback.release()
        super.onDestroy()
    }
}