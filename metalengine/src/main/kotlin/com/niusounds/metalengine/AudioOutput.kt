package com.niusounds.metalengine

import com.niusounds.metalengine.impl.AudioOutputImpl
import java.nio.ByteBuffer

interface AudioOutput {
    val bufferCount: Int
    val frameSize: Int
    val channels: Int
    val sampleRate: Int

    fun release()
    fun start()
    fun stop()
    fun writeFloats(buffer: ByteBuffer)

    companion object {
        fun create(
            sampleRate: Int,
            frameSize: Int,
            channels: Int,
            bufferCount: Int = sampleRate / frameSize,
            lowLatency: Boolean = true,
        ): AudioOutput {
            return AudioOutputImpl(
                sampleRate = sampleRate,
                frameSize = frameSize,
                channels = channels,
                bufferCount = bufferCount,
                lowLatency = lowLatency
            )
        }
    }
}