#include <jni.h>
#include <string>
#include <oboe/Oboe.h>
#include <android/log.h>
#include "readerwriterqueue.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_niusounds_oboetest_MainActivity_stringFromJNI(JNIEnv *env, jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

class EnqueueInfo {
public:
    int startPos;
};

class AudioEngine : public oboe::AudioStreamDataCallback {
public:
    explicit AudioEngine(int frameSize, int writeBufferCount) {
        this->frameSize = frameSize;
        audioWriteBufferSize = frameSize * writeBufferCount;
        recordBuffer = new float[frameSize];
        audioWriteBuffer = new float[audioWriteBufferSize];
        queue = moodycamel::ReaderWriterQueue<EnqueueInfo>(writeBufferCount);
    }

    ~AudioEngine() override {
        delete recordBuffer;
        delete audioWriteBuffer;
    }

    // must contains frameSize audio data.
    bool enqueue(const float *audioData) {
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
        return queue.try_enqueue(info);

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

AudioEngine *myCallback = nullptr;

void cleanup() {
    if (myCallback != nullptr) {
        delete myCallback;
        myCallback = nullptr;
    }
}

void startAudio(int frameSize, int bufferCount) {
    oboe::AudioStreamBuilder inBuilder;
    inBuilder.setDirection(oboe::Direction::Input)
            ->setSampleRate(48000)
            ->setBufferCapacityInFrames(frameSize * 4 /* extra buffer*/)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Shared)
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(oboe::Mono);

    myCallback = new AudioEngine(frameSize, bufferCount);
    oboe::Result inResult = inBuilder.openManagedStream(myCallback->inStream);
    if (inResult != oboe::Result::OK) {
        delete myCallback;
        __android_log_print(ANDROID_LOG_ERROR, "OboeTest", "open inStream failed!");
        return;
    }

    oboe::AudioStreamBuilder outBuilder;
    outBuilder.setDirection(oboe::Direction::Output)
            ->setSampleRate(48000)
            ->setFramesPerCallback(frameSize)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Shared)
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(oboe::Mono)
            ->setDataCallback(myCallback);

    oboe::Result outResult = outBuilder.openManagedStream(myCallback->outStream);
    if (outResult != oboe::Result::OK) {
        delete myCallback;
        __android_log_print(ANDROID_LOG_ERROR, "OboeTest", "open outStream failed!");
        return;
    }

    myCallback->inStream->requestStart();
    myCallback->outStream->requestStart();
}

extern "C" JNIEXPORT void JNICALL
Java_com_niusounds_oboetest_MainActivity_nativeAudio(JNIEnv *env, jobject /* this */,
                                                     jint frameSize, jint bufferCount) {
    startAudio(frameSize, bufferCount);
}

extern "C" JNIEXPORT void JNICALL
Java_com_niusounds_oboetest_MainActivity_release(JNIEnv *env, jobject /* this */) {
    cleanup();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_niusounds_oboetest_MainActivity_writeFloats(JNIEnv *env, jobject, jobject buffer) {
    auto bufferPtr = static_cast<float *>(env->GetDirectBufferAddress(buffer));
    return myCallback->enqueue(bufferPtr);
}

