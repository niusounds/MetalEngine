#include <jni.h>
#include <string>
#include <oboe/Oboe.h>
#include <android/log.h>
#include "readerwriterqueue.h"

#define LOGE(msg) __android_log_print(ANDROID_LOG_ERROR, "MetalEngine", msg);

class EnqueueInfo {
public:
    int startPos;
};

class MetalEngine : public oboe::AudioStreamDataCallback {
public:
    explicit MetalEngine(int frameSize, int writeBufferCount) {
        this->frameSize = frameSize;
        audioWriteBufferSize = frameSize * writeBufferCount;
        recordBuffer = new float[frameSize];
        audioWriteBuffer = new float[audioWriteBufferSize];
        queue = moodycamel::ReaderWriterQueue<EnqueueInfo>(writeBufferCount);
    }

    ~MetalEngine() override {
        delete recordBuffer;
        delete audioWriteBuffer;
    }

    // must contains frameSize audio data.
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

//        if (endPos > startPos) {
//            memcpy(audioWriteBuffer + startPos * sizeof(float), audioData, numFrames);
//        } else {
//            int firstWriteSize = audioWriteBufferSize - startPos;
//            memcpy(audioWriteBuffer + startPos * sizeof(float), audioData, firstWriteSize);
//            int secondWriteSize = numFrames - firstWriteSize;
//            memcpy(audioWriteBuffer, audioData + firstWriteSize, firstWriteSize);
//
//        }
    }

    oboe::DataCallbackResult
    onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) override {
        // recordBufferにマイクから読み込み
        auto result = inStream->read(recordBuffer, numFrames, 0);
        if (result == oboe::Result::OK) {
            int readSize = result.value();

            // replace the missing data with silence
            if (readSize != numFrames) {
                memset(static_cast<float *>(recordBuffer) +
                       readSize * inStream->getBytesPerSample(), 0,
                       (numFrames - readSize) * inStream->getBytesPerFrame());
            }

            // TODO audioDataに出力データを詰める
            auto *output = (float *) audioData;
            int numChannels = outStream->getChannelCount();

            EnqueueInfo audioDataRange{};
            if (queue.try_dequeue(audioDataRange)) {
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
                // output is muted
                memset(output, 0, numFrames * inStream->getBytesPerFrame());
            }
            // Test: Noise
//            for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
//                for (int channelIndex = 0; channelIndex < numChannels; channelIndex++) {
//                    auto noise = (float) (drand48() - 0.5);
//                    *output++ = noise;
//                }
//            }
            // Test: Mic Input
//            auto *input = recordBuffer;
//            for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
//                for (int channelIndex = 0; channelIndex < numChannels; channelIndex++) {
//                    *output++ = *input++;
//                }
//            }

            // TODO AEC

            return oboe::DataCallbackResult::Continue;
        } else {
            // Read Error!
            return oboe::DataCallbackResult::Stop;
        }

    }

public:
    oboe::ManagedStream inStream;
    oboe::ManagedStream outStream;
private:
    int frameSize;
    float *recordBuffer;
    float *audioWriteBuffer;
    int audioWriteBufferPos = 0;
    int audioWriteBufferSize;
    moodycamel::ReaderWriterQueue<EnqueueInfo> queue;
};

extern "C" JNIEXPORT jlong JNICALL
Java_com_niusounds_metalengine_MetalEngine_nativeInit(JNIEnv *env, jobject /* this */,
                                                      jint sampleRate, jint frameSize,
                                                      jint channels, jint bufferCount,
                                                      jboolean lowLatency) {
    auto performanceMode = lowLatency ? oboe::PerformanceMode::LowLatency
                                      : oboe::PerformanceMode::PowerSaving;
    oboe::AudioStreamBuilder inBuilder;
    inBuilder.setDirection(oboe::Direction::Input)
            ->setSampleRate(sampleRate)
            ->setBufferCapacityInFrames(frameSize * 4 /* extra buffer*/)
            ->setPerformanceMode(performanceMode)
            ->setSharingMode(oboe::SharingMode::Shared)
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(channels);

    auto engine = new MetalEngine(frameSize, bufferCount);
    oboe::Result inResult = inBuilder.openManagedStream(engine->inStream);
    if (inResult != oboe::Result::OK) {
        delete engine;
        LOGE("open inStream failed!");
        return 0;
    }

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
Java_com_niusounds_metalengine_MetalEngine_start(JNIEnv *env, jobject /* this */, jlong nativePtr) {
    auto engine = reinterpret_cast<MetalEngine *>(nativePtr);
    engine->outStream->requestStart();
    engine->inStream->requestStart();
}

extern "C" JNIEXPORT void JNICALL
Java_com_niusounds_metalengine_MetalEngine_release(JNIEnv *env, jobject /* this */,
                                                   jlong nativePtr) {
    auto engine = reinterpret_cast<MetalEngine *>(nativePtr);
    delete engine;
}

extern "C" JNIEXPORT void JNICALL
Java_com_niusounds_metalengine_MetalEngine_writeFloats(JNIEnv *env, jobject/*this*/,
                                                       jlong nativePtr, jobject buffer) {
    auto engine = reinterpret_cast<MetalEngine *>(nativePtr);
    auto bufferPtr = static_cast<float *>(env->GetDirectBufferAddress(buffer));
    engine->enqueue(bufferPtr);
}

