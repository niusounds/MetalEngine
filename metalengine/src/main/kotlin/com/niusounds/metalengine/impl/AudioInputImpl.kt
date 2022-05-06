package com.niusounds.metalengine.impl

import com.niusounds.metalengine.AudioInput
import java.nio.ByteBuffer

internal class AudioInputImpl(
    override val sampleRate: Int,
    override val frameSize: Int,
    override val channels: Int,
    val lowLatency: Boolean = true,
) : AudioInput {
    /**
     * Holds native AudioOutput's pointer.
     */
    private var nativePtr: Long

    private var running = false

    init {
        nativePtr = nativeInit(sampleRate, frameSize, channels, lowLatency)
        if (nativePtr == 0L) error("Cannot initialize native audio engine.")
    }

    override fun release() {
        if (nativePtr != 0L) {
            release(nativePtr)
            nativePtr = 0
        }
    }

    /**
     * Start audio processing.
     */
    override fun start() {
        if (running) return
        running = true

        start(nativePtr)
    }

    override fun stop() {
        if (!running) return
        running = false

        stop(nativePtr)
    }

    /**
     * Write output audio data.
     * [buffer] must contains float pcm and its sample count must be equal to frameSize * channels.
     * if engine is initialized with frameSize: 960 channels: 2, buffer must be filled with 1920 float samples. (frameSize * channels)
     */
    override fun readFloats(buffer: ByteBuffer) {
        if (!running) return
        readFloats(nativePtr, buffer)
    }

    private external fun nativeInit(
        sampleRate: Int,
        frameSize: Int,
        channels: Int,
        lowLatency: Boolean,
    ): Long

    private external fun start(nativePtr: Long)
    private external fun stop(nativePtr: Long)
    private external fun release(nativePtr: Long)
    private external fun readFloats(nativePtr: Long, buffer: ByteBuffer)

    companion object {
        init {
            System.loadLibrary("metalengine")
        }
    }
}