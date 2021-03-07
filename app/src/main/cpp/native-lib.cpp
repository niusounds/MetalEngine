#include <jni.h>
#include <string>
#include <oboe/Oboe.h>
#include <android/log.h>

extern "C" JNIEXPORT jstring JNICALL
Java_com_niusounds_oboetest_MainActivity_stringFromJNI(JNIEnv *env, jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

class MyCallback : public oboe::AudioStreamDataCallback {
public:
    oboe::DataCallbackResult
    onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) override {
//        auto *outputData = static_cast<float *>(audioData);
        auto readSize = inStream->read(audioData, numFrames, 0);

        return oboe::DataCallbackResult::Continue;
    }

public:
    oboe::ManagedStream inStream;
    oboe::ManagedStream outStream;
};

MyCallback *myCallback = nullptr;

void cleanup() {
    if (myCallback != nullptr) {
        delete myCallback;
        myCallback = nullptr;
    }
}

void startAudio() {
    oboe::AudioStreamBuilder inBuilder;
    inBuilder.setDirection(oboe::Direction::Input)
            ->setSampleRate(48000)
            ->setBufferCapacityInFrames(960 * 2/*ch*/* 4 /* extra buffer*/)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Shared)
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(oboe::Stereo);

    myCallback = new MyCallback();
    oboe::Result inResult = inBuilder.openManagedStream(myCallback->inStream);
    if (inResult != oboe::Result::OK) {
        delete myCallback;
        __android_log_print(ANDROID_LOG_ERROR, "OboeTest", "open inStream failed!");
        return;
    }

    oboe::AudioStreamBuilder outBuilder;
    outBuilder.setDirection(oboe::Direction::Output)
            ->setSampleRate(48000)
            ->setFramesPerCallback(960 * 2/*ch*/)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Shared)
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(oboe::Stereo)
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
Java_com_niusounds_oboetest_MainActivity_nativeAudio(JNIEnv *env, jobject /* this */) {
    startAudio();
}

extern "C" JNIEXPORT void JNICALL
Java_com_niusounds_oboetest_MainActivity_release(JNIEnv *env, jobject /* this */) {
    cleanup();
}

