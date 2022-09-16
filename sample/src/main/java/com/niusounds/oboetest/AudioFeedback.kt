package com.niusounds.oboetest

import com.niusounds.metalengine.AudioInput
import com.niusounds.metalengine.AudioOutput
import kotlin.concurrent.thread

class AudioFeedback(
    private val input: AudioInput,
    private val output: AudioOutput,
) {

    private var thread: Thread? = null

    fun start() {
        input.start()
        output.start()
        startWriting()
    }

    private fun startWriting() {
        check(thread == null)
        thread = thread {
            runCatching {
                android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO)
                val data = Array(output.bufferCount) {
                    FloatArray(output.frameSize * output.channels)
                    // or ByteBuffer.allocateDirect(output.frameSize * output.channels * Float.SIZE_BYTES).order(ByteOrder.nativeOrder())
                }

                var index = 0
                while (!Thread.interrupted()) {
                    val buffer = data[index]

                    input.read(buffer)
                    // or input.readFloats(buffer)
                    output.write(buffer)
                    // or output.writeFloats(buffer)

                    index++
                    if (index >= data.size) {
                        index = 0
                    }
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
        input.stop()
        output.stop()
    }

    fun release() {
        input.release()
        output.release()
    }
}
