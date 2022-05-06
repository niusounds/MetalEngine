package com.niusounds.oboetest

import com.niusounds.metalengine.AudioOutput
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.concurrent.thread
import kotlin.math.sin

class ToneGenerator(
    private val engine: AudioOutput,
) {

    private var thread: Thread? = null

    fun start() {
        engine.start()
        startWriting()
    }

    private fun startWriting() {
        check(thread == null)
        thread = thread {
            runCatching {
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
    }

    fun stop() {
        thread?.let {
            it.interrupt()
            it.join()
        }
        thread = null
        engine.stop()
    }

    fun release() {
        engine.release()
    }
}