#include <jni.h>
#include <string>
#include <oboe/Oboe.h>
#include <android/log.h>
#include "readerwriterqueue.h"

#define LOGE(msg) __android_log_print(ANDROID_LOG_ERROR, "AudioOutput", msg);

class EnqueueInfo {
public:
    int startPos;
};

class AudioOutput : public oboe::AudioStreamDataCallback {
public:
    explicit AudioOutput(int frameSize, int writeBufferCount) {
        this->frameSize = frameSize;
        audioWriteBufferSize = frameSize * writeBufferCount;
        audioWriteBuffer = new float[audioWriteBufferSize];
        queue = moodycamel::ReaderWriterQueue<EnqueueInfo>(writeBufferCount);
    }

    ~AudioOutput() override {
        delete audioWriteBuffer;
    }

    // must contains frameSize audio data.
    //
    void enqueue(const float *audioData) {
        int startPos = audioWriteBufferPos;
        int index = startPos;
        for (int i = 0; i < frameSize; ++i) {
            audioWriteBuffer[index] = audioData[i];
            ++index;
            if (index >= audioWriteBufferSize) {
                index = 0;
            }
        }
        audioWriteBufferPos = index; // next enqueue pos

        auto info = EnqueueInfo();
        info.startPos = startPos;
        while (!queue.try_enqueue(info));
    }

    oboe::DataCallbackResult
    onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) override {

        auto *output = (float *) audioData;
        int numChannels = outStream->getChannelCount();

        EnqueueInfo audioDataRange{};
        if (queue.try_dequeue(audioDataRange)) {
            // Fill `output` with PCM data enqueued by `enqueue()`.
            int index = audioDataRange.startPos;
            for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
                for (int channelIndex = 0; channelIndex < numChannels; channelIndex++) {
                    *output++ = audioWriteBuffer[index];
                    ++index;
                    if (index >= audioWriteBufferSize) {
                        index = 0;
                    }
                }
            }
        } else {
            // queue does not contain EnqueueInfo so output has to be zero filled (muted).
            memset(output, 0, numFrames * outStream->getBytesPerFrame());
        }

        return oboe::DataCallbackResult::Continue;
    }

public:
    oboe::ManagedStream outStream;
private:
    int frameSize;
    float *audioWriteBuffer;

    int audioWriteBufferPos = 0;
    int audioWriteBufferSize;
    moodycamel::ReaderWriterQueue<EnqueueInfo> queue;
};

extern "C" JNIEXPORT jlong JNICALL
Java_com_niusounds_metalengine_impl_AudioOutputImpl_nativeInit(JNIEnv *env, jobject /* this */,
                                                               jint sampleRate, jint frameSize,
                                                               jint channels, jint bufferCount,
                                                               jboolean lowLatency) {
    auto engine = new AudioOutput(frameSize, bufferCount);

    auto performanceMode = lowLatency ? oboe::PerformanceMode::LowLatency
                                      : oboe::PerformanceMode::PowerSaving;

    oboe::AudioStreamBuilder outBuilder;
    outBuilder.setDirection(oboe::Direction::Output)
            ->setSampleRate(sampleRate)
            ->setFramesPerCallback(frameSize)
            ->setPerformanceMode(performanceMode)
            ->setSharingMode(oboe::SharingMode::Shared)
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(channels)
            ->setDataCallback(engine);

    oboe::Result outResult = outBuilder.openManagedStream(engine->outStream);
    if (outResult != oboe::Result::OK) {
        delete engine;
        LOGE("open outStream failed!");
        return 0;
    }

    return reinterpret_cast<jlong>(engine);
}

extern "C" JNIEXPORT void JNICALL
Java_com_niusounds_metalengine_impl_AudioOutputImpl_start(JNIEnv *env, jobject /* this */, jlong nativePtr) {
    auto engine = reinterpret_cast<AudioOutput *>(nativePtr);
    engine->outStream->requestStart();
}

extern "C" JNIEXPORT void JNICALL
Java_com_niusounds_metalengine_impl_AudioOutputImpl_stop(JNIEnv *env, jobject /* this */, jlong nativePtr) {
    auto engine = reinterpret_cast<AudioOutput *>(nativePtr);
    engine->outStream->requestStop();
}

extern "C" JNIEXPORT void JNICALL
Java_com_niusounds_metalengine_impl_AudioOutputImpl_release(JNIEnv *env, jobject /* this */,
                                                            jlong nativePtr) {
    auto engine = reinterpret_cast<AudioOutput *>(nativePtr);
    delete engine;
}

extern "C" JNIEXPORT void JNICALL
Java_com_niusounds_metalengine_impl_AudioOutputImpl_writeFloats(JNIEnv *env, jobject/*this*/,
                                                                jlong nativePtr, jobject buffer) {
    auto engine = reinterpret_cast<AudioOutput *>(nativePtr);
    auto bufferPtr = static_cast<float *>(env->GetDirectBufferAddress(buffer));
    engine->enqueue(bufferPtr);
}
