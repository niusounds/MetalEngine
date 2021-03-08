package com.niusounds.oboetest

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.View
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.concurrent.thread
import kotlin.math.sin

class MainActivity : AppCompatActivity() {
    private val recordPermission = Manifest.permission.RECORD_AUDIO
    private var thread: Thread? = null
    private val frameSize = 960
    private val bufferCount = 16

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
            nativeAudio(frameSize, bufferCount)
            startWriting()
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
                        nativeAudio(frameSize, bufferCount)
                        startWriting()
                    }
                }
            }
        }
    }

    private fun startWriting() {
        thread = thread {
            val data = Array(bufferCount) {
                ByteBuffer.allocateDirect(frameSize * Float.SIZE_BYTES).order(
                    ByteOrder.nativeOrder()
                )
            }

            val floatBuffers = data.map { it.asFloatBuffer() }

            var index = 0
            var t = 0.0
            val dt = 1.0 / 48000.0
            while (!Thread.interrupted()) {
                val buffer = floatBuffers[index]

                for (i in 0 until buffer.limit()) {
                    buffer.put(i, sin(t * 2.0 * Math.PI * 440.0).toFloat())
                    t += dt
                }

                while (!writeFloats(data[index])) {
                    Thread.sleep(frameSize / 48000L * 1000L)
                }

                index++
                if (index >= data.size) {
                    index = 0
                }

                Thread.sleep((buffer.limit() * 900 / 48000).toLong())
            }
        }
    }

    override fun onDestroy() {
        thread?.let { it.interrupt(); it.join() }
        release()
        super.onDestroy()
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String
    external fun nativeAudio(frameSize: Int, bufferCount: Int)
    external fun release()
    external fun writeFloats(buffer: ByteBuffer): Boolean

    companion object {
        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("app")
        }
    }

}