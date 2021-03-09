package com.niusounds.oboetest

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.niusounds.metalengine.MetalEngine
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.concurrent.thread
import kotlin.math.sin

class MainActivity : AppCompatActivity() {
    private val recordPermission = Manifest.permission.RECORD_AUDIO
    private var thread: Thread? = null
    private val engine: MetalEngine by lazy { MetalEngine(48000, 1920, 1, 16) }

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
            engine.start()
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
                        engine.start()
                        startWriting()
                    }
                }
            }
        }
    }

    private fun startWriting() {
        thread = thread {
            android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO)
            val data = Array(engine.bufferCount) {
                ByteBuffer.allocateDirect(engine.frameSize * engine.channels * Float.SIZE_BYTES)
                    .order(ByteOrder.nativeOrder())
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

                engine.writeFloats(data[index])

                index++
                if (index >= data.size) {
                    index = 0
                }

                Thread.sleep((buffer.limit() * 900 / engine.sampleRate).toLong())
            }
        }
    }

    override fun onDestroy() {
        thread?.let { it.interrupt(); it.join() }
        engine.release()
        super.onDestroy()
    }
}