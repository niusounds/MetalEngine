#include <jni.h>
#include <string>
#include <oboe/Oboe.h>
#include <android/log.h>
#include "readerwriterqueue.h"

#define LOGE(msg) __android_log_print(ANDROID_LOG_ERROR, "AudioInput", msg);

class EnqueueInfo {
public:
    int startPos;
};

class AudioInput : public oboe::AudioStreamDataCallback {
public:
    explicit AudioInput() = default;

    ~AudioInput() override = default;

    void read(float *audioData) {
        readBuffer = audioData;

        // wait until readToExternalBuffer is set to false in onAudioReady
        while (readBuffer != nullptr);
    }

    oboe::DataCallbackResult
    onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) override {

        // read to externalReadBuffer
        if (readBuffer != nullptr) {
            memcpy(readBuffer, (float *) audioData, numFrames * inStream->getBytesPerFrame());
            readBuffer = nullptr;
        }

        return oboe::DataCallbackResult::Continue;
    }

public:
    oboe::ManagedStream inStream;
private:
    std::atomic<float *> readBuffer{};
};

extern "C" JNIEXPORT jlong JNICALL
Java_com_niusounds_metalengine_impl_AudioInputImpl_nativeInit(JNIEnv *env, jobject, jint sampleRate,
                                                              jint frameSize, jint channels,
                                                              jboolean lowLatency) {
    auto input = new AudioInput();
    auto performanceMode = lowLatency ? oboe::PerformanceMode::LowLatency
                                      : oboe::PerformanceMode::PowerSaving;
    oboe::AudioStreamBuilder inBuilder;
    inBuilder.setDirection(oboe::Direction::Input)
            ->setSampleRate(sampleRate)
            ->setBufferCapacityInFrames(frameSize * 4 /* extra buffer*/)
            ->setPerformanceMode(performanceMode)
            ->setSharingMode(oboe::SharingMode::Shared)
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(channels)
            ->setFramesPerCallback(frameSize)
            ->setDataCallback(input);

    oboe::Result inResult = inBuilder.openManagedStream(input->inStream);
    if (inResult != oboe::Result::OK) {
        delete input;
        LOGE("open inStream failed!");
        return 0;
    }

    return reinterpret_cast<jlong>(input);
}

extern "C" JNIEXPORT void JNICALL
Java_com_niusounds_metalengine_impl_AudioInputImpl_start(JNIEnv *env, jobject /* this */,
                                                         jlong nativePtr) {
    auto input = reinterpret_cast<AudioInput *>(nativePtr);
    input->inStream->requestStart();
}

extern "C" JNIEXPORT void JNICALL
Java_com_niusounds_metalengine_impl_AudioInputImpl_stop(JNIEnv *env, jobject /* this */,
                                                        jlong nativePtr) {
    auto input = reinterpret_cast<AudioInput *>(nativePtr);
    input->inStream->requestStop();
}

extern "C" JNIEXPORT void JNICALL
Java_com_niusounds_metalengine_impl_AudioInputImpl_release(JNIEnv *env, jobject /* this */,
                                                           jlong nativePtr) {
    auto input = reinterpret_cast<AudioInput *>(nativePtr);
    delete input;
}

extern "C" JNIEXPORT void JNICALL
Java_com_niusounds_metalengine_impl_AudioInputImpl_read(JNIEnv *env, jobject/*this*/,
                                                        jlong nativePtr, jfloatArray audioData) {
    auto input = reinterpret_cast<AudioInput *>(nativePtr);
    auto audioDataPtr = env->GetFloatArrayElements(audioData, 0);
    input->read(audioDataPtr);
    env->ReleaseFloatArrayElements(audioData, audioDataPtr, 0);
}

extern "C" JNIEXPORT void JNICALL
Java_com_niusounds_metalengine_impl_AudioInputImpl_readFloats(JNIEnv *env, jobject/*this*/,
                                                              jlong nativePtr, jobject buffer) {
    auto input = reinterpret_cast<AudioInput *>(nativePtr);
    auto bufferPtr = static_cast<float *>(env->GetDirectBufferAddress(buffer));
    input->read(bufferPtr);
}

