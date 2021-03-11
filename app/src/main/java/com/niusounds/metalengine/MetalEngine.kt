package com.niusounds.metalengine

import android.os.Process
import java.nio.ByteBuffer
import kotlin.concurrent.thread

class MetalEngine(
    val sampleRate: Int,
    val frameSize: Int,
    val channels: Int,
    val bufferCount: Int = sampleRate / frameSize,
    val lowLatency: Boolean = true,
) {
    private var nativePtr: Long

    private var running = false
    private val inBuffer = ByteBuffer.allocateDirect(frameSize * channels * Float.SIZE_BYTES)
    private var readThread: Thread? = null

    init {
        nativePtr = nativeInit(sampleRate, frameSize, channels, bufferCount, lowLatency)
        if (nativePtr == 0L) error("Cannot initialize native audio engine.")
    }

    fun release() {
        if (nativePtr != 0L) {
            release(nativePtr)
            nativePtr = 0
        }
    }

    /**
     * Start audio processing.
     */
    fun start() {
        if (running) return
        running = true

        start(nativePtr)
        startReadThread()
    }

    private fun startReadThread() {
        readThread = thread {
            Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO)

            val audioRecordData = AudioRecordData(sampleRate, frameSize, channels, inBuffer)
            while (!Thread.interrupted()) {
                readFloats(nativePtr, inBuffer)
                // TODO notify audioRecordData
            }
        }
    }

    /**
     * Write output audio data.
     * [buffer] must contains float pcm and its sample count must be equal to frameSize * channels.
     * if engine is initialized with frameSize: 960 channels: 2, buffer must be filled with 1920 float samples. (frameSize * channels)
     */
    fun writeFloats(buffer: ByteBuffer) = writeFloats(nativePtr, buffer)

    private external fun nativeInit(
        sampleRate: Int,
        frameSize: Int,
        channels: Int,
        bufferCount: Int,
        lowLatency: Boolean,
    ): Long

    private external fun start(nativePtr: Long)
    private external fun release(nativePtr: Long)
    private external fun writeFloats(nativePtr: Long, buffer: ByteBuffer)
    private external fun readFloats(nativePtr: Long, buffer: ByteBuffer)

    companion object {
        init {
            System.loadLibrary("metalengine")
        }
    }
}