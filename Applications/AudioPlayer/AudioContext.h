#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

#include "AudioTrack.h"

#define AUDIOCONTEXT_NUM_SAMPLE_BUFFERS 16

class AudioContext {
    friend void DecodeAudio(AudioContext*);
    friend void PlayAudio(AudioContext*);

public:
    struct SampleBuffer {
        uint8_t* data;
        int samples;

        // Timestamp in seconds of last frame in buffer
        float timestamp;
    };

    AudioContext();

    inline bool IsAudioPlaying() const { return m_isDecoderRunning; }

    float PlaybackProgress() const;
    const TrackInfo* CurrentTrack() const { return m_currentTrack; }

    void PlaybackStart();
    void PlaybackPause();
    void PlaybackStop();
    void PlaybackSeek(float timestamp);

    int PlayTrack(TrackInfo* info);
    int LoadTrack(std::string filepath, TrackInfo* info);

    SampleBuffer sampleBuffers[AUDIOCONTEXT_NUM_SAMPLE_BUFFERS];
    ssize_t samplesPerBuffer;
    // Ring buffer of SampleBuffers
    // Index of the last valid sample buffer
    int lastSampleBuffer;
    // The currently processed sample buffer
    int currentSampleBuffer;
    std::mutex sampleBuffersLock;
    // Decoder waits for a buffer to be processed
    std::condition_variable decoderWaitCondition;
    int numValidBuffers;

private:
    void GetTrackInfo(struct AVFormatContext* fmt, TrackInfo* info);

    inline int DecoderNextBuffer() const {return (lastSampleBuffer + 1) % AUDIOCONTEXT_NUM_SAMPLE_BUFFERS; }
    inline void FlushSampleBuffers() {
        for (int i = 0; i < AUDIOCONTEXT_NUM_SAMPLE_BUFFERS; i++) {
            sampleBuffers[i].samples = 0;
            sampleBuffers[i].timestamp = 0;
        }
    }

    // File descriptor for pcm output
    int m_pcmOut;
    int m_pcmSampleRate;
    int m_pcmChannels;
    int m_pcmBitDepth;

    TrackInfo* m_currentTrack;

    // Thread which writes PCM samples to the audio driver,
    // reduces audio lag and prevents blocking the main thread
    // so the decoder and GUI can run
    std::thread m_playbackThread;

    // Decoder runs parallel to playback and GUI threads
    std::thread m_decoderThread;
    // Held by the decoderThread whilst it is running
    std::mutex m_decoderLock;

    bool m_isDecoderRunning = false;
    
    bool m_requestSeek = false;
    // Timestamp in seconds of where to seek to
    float m_seekTimestamp;
    // Last timestamp played by the playback thread
    float m_lastTimestamp;

    struct AVFormatContext* m_avfmt = nullptr;
    struct AVCodecContext* m_avcodec = nullptr;

    struct AVStream* m_currentStream = nullptr;
    int m_currentStreamIndex = 0;
};