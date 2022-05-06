package com.niusounds.metalengine

import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.FloatBuffer

class AudioRecordData(
    val sampleRate: Int,
    val frameSize: Int,
    val channels: Int,
    val buffer: ByteBuffer
) {
    val floatBuffer: FloatBuffer by lazy {
        buffer.order(ByteOrder.nativeOrder()).asFloatBuffer()
    }
}