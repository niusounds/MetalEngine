package com.niusounds.metalengine

import java.nio.ByteBuffer

class MetalEngine(
    val sampleRate: Int,
    val frameSize: Int,
    val channels: Int,
    val bufferCount: Int = sampleRate / frameSize,
    val lowLatency: Boolean = true,
) {
    private var nativePtr: Long

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
    fun start() = start(nativePtr)

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

    companion object {
        init {
            System.loadLibrary("metalengine")
        }
    }
}