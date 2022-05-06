# 🤘METAL ENGINE🤘

Blazing fast AudioRecord/AudioTrack for Android powered by [Oboe audio library](https://developer.android.com/games/sdk/oboe).

## Recording

```kotlin
// 1. Create AudioInput instance
val input = AudioInput.create(
    sampleRate = 48000,
    frameSize = 1920,
    channels = 1,
)

// 2. Start audio engine
input.start()

// 3. Create audio buffer
val buffer = ByteBuffer.allocateDirect(input.frameSize * input.channels * Float.SIZE_BYTES).order(ByteOrder.nativeOrder())
val floatBuffer = buffer.asFloatBuffer()

// 4. Start audio reading thread
audioThread = thread {
    android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO)

    try {
        while (!Thread.interrupted()) {
            input.readFloats(buffer)

            // Now floatBuffer is filled by audio data.
            // Process audio data here.
        }
    } catch(e: Exception) {
        // thread is interrupted or something wrong.
    }
}

// 5. Stop audio engine. Do not forget to stop audio reading thread too.
input.stop()
audioThread.interrupt()

// 6. Finally, call release() to release native resources.
input.release()
```

## Playing

```kotlin
// 1. Create AudioOutput instance
val output = AudioOutput.create(
    sampleRate = 48000,
    frameSize = 1920,
    channels = 1,
    bufferCount = 16,
)

// 2. Start audio engine
output.start()

// 3. Create audio buffer
val byteBuffers = Array(output.bufferCount) {
    ByteBuffer.allocateDirect(output.frameSize * output.channels * Float.SIZE_BYTES).order(ByteOrder.nativeOrder())
}

val floatBuffers = byteBuffers.map { it.asFloatBuffer() }

// 4. Start audio playing thread
audioThread = thread {
    android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO)

    try {
        var index = 0
        while (!Thread.interrupted()) {
            val floatBuffer = floatBuffers[index]

            // Fill floatBuffer with audio data

            output.writeFloats(byteBuffers[index])

            index++
            if (index >= byteBuffers.size) {
                index = 0
            }
        }
    } catch(e: Exception) {
        // thread is interrupted or something wrong.
    }
}

// 5. Stop audio engine. Do not forget to stop audio playing thread too.
output.stop()
audioThread.interrupt()

// 6. Finally, call release() to release native resources.
output.release()
```
