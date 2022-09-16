package com.niusounds.metalengine

import com.niusounds.metalengine.impl.AudioInputImpl
import java.nio.ByteBuffer

interface AudioInput {
    val frameSize: Int
    val channels: Int
    val sampleRate: Int

    fun release()
    fun start()
    fun stop()
    fun read(audioData: FloatArray)
    fun readFloats(buffer: ByteBuffer)

    companion object {
        fun create(
            sampleRate: Int,
            frameSize: Int,
            channels: Int,
            lowLatency: Boolean = true,
        ): AudioInput {
            return AudioInputImpl(
                sampleRate = sampleRate,
                frameSize = frameSize,
                channels = channels,
                lowLatency = lowLatency
            )
        }
    }
}