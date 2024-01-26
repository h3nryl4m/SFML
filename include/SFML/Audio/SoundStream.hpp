////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2024 Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Audio/Export.hpp>

#include <SFML/Audio/SoundSource.hpp>

#include <SFML/System/Time.hpp>

#include <mutex>
#include <thread>

#include <cstdlib>


namespace sf
{
////////////////////////////////////////////////////////////
/// \brief Abstract base class for streamed audio sources
///
////////////////////////////////////////////////////////////
class SFML_AUDIO_API SoundStream : public SoundSource
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Structure defining a chunk of audio data to stream
    ///
    ////////////////////////////////////////////////////////////
    struct Chunk
    {
        const std::int16_t* samples;     //!< Pointer to the audio samples
        std::size_t         sampleCount; //!< Number of samples pointed by Samples
    };

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~SoundStream() override;

    ////////////////////////////////////////////////////////////
    /// \brief Start or resume playing the audio stream
    ///
    /// This function starts the stream if it was stopped, resumes
    /// it if it was paused, and restarts it from the beginning if
    /// it was already playing.
    /// This function uses its own thread so that it doesn't block
    /// the rest of the program while the stream is played.
    ///
    /// \see pause, stop
    ///
    ////////////////////////////////////////////////////////////
    void play() override;

    ////////////////////////////////////////////////////////////
    /// \brief Pause the audio stream
    ///
    /// This function pauses the stream if it was playing,
    /// otherwise (stream already paused or stopped) it has no effect.
    ///
    /// \see play, stop
    ///
    ////////////////////////////////////////////////////////////
    void pause() override;

    ////////////////////////////////////////////////////////////
    /// \brief Stop playing the audio stream
    ///
    /// This function stops the stream if it was playing or paused,
    /// and does nothing if it was already stopped.
    /// It also resets the playing position (unlike pause()).
    ///
    /// \see play, pause
    ///
    ////////////////////////////////////////////////////////////
    void stop() override;

    ////////////////////////////////////////////////////////////
    /// \brief Return the number of channels of the stream
    ///
    /// 1 channel means a mono sound, 2 means stereo, etc.
    ///
    /// \return Number of channels
    ///
    ////////////////////////////////////////////////////////////
    unsigned int getChannelCount() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the stream sample rate of the stream
    ///
    /// The sample rate is the number of audio samples played per
    /// second. The higher, the better the quality.
    ///
    /// \return Sample rate, in number of samples per second
    ///
    ////////////////////////////////////////////////////////////
    unsigned int getSampleRate() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the current status of the stream (stopped, paused, playing)
    ///
    /// \return Current status
    ///
    ////////////////////////////////////////////////////////////
    Status getStatus() const override;

    ////////////////////////////////////////////////////////////
    /// \brief Change the current playing position of the stream
    ///
    /// The playing position can be changed when the stream is
    /// either paused or playing. Changing the playing position
    /// when the stream is stopped has no effect, since playing
    /// the stream would reset its position.
    ///
    /// \param timeOffset New playing position, from the beginning of the stream
    ///
    /// \see getPlayingOffset
    ///
    ////////////////////////////////////////////////////////////
    void setPlayingOffset(Time timeOffset);

    ////////////////////////////////////////////////////////////
    /// \brief Get the current playing position of the stream
    ///
    /// \return Current playing position, from the beginning of the stream
    ///
    /// \see setPlayingOffset
    ///
    ////////////////////////////////////////////////////////////
    Time getPlayingOffset() const;

    ////////////////////////////////////////////////////////////
    /// \brief Set whether or not the stream should loop after reaching the end
    ///
    /// If set, the stream will restart from beginning after
    /// reaching the end and so on, until it is stopped or
    /// setLoop(false) is called.
    /// The default looping state for streams is false.
    ///
    /// \param loop True to play in loop, false to play once
    ///
    /// \see getLoop
    ///
    ////////////////////////////////////////////////////////////
    void setLoop(bool loop);

    ////////////////////////////////////////////////////////////
    /// \brief Tell whether or not the stream is in loop mode
    ///
    /// \return True if the stream is looping, false otherwise
    ///
    /// \see setLoop
    ///
    ////////////////////////////////////////////////////////////
    bool getLoop() const;

protected:
    enum
    {
        NoLoop = -1 //!< "Invalid" endSeeks value, telling us to continue uninterrupted
    };

    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    /// This constructor is only meant to be called by derived classes.
    ///
    ////////////////////////////////////////////////////////////
    SoundStream() = default;

    ////////////////////////////////////////////////////////////
    /// \brief Define the audio stream parameters
    ///
    /// This function must be called by derived classes as soon
    /// as they know the audio settings of the stream to play.
    /// Any attempt to manipulate the stream (play(), ...) before
    /// calling this function will fail.
    /// It can be called multiple times if the settings of the
    /// audio stream change, but only when the stream is stopped.
    ///
    /// \param channelCount Number of channels of the stream
    /// \param sampleRate   Sample rate, in samples per second
    ///
    ////////////////////////////////////////////////////////////
    void initialize(unsigned int channelCount, unsigned int sampleRate);

    ////////////////////////////////////////////////////////////
    /// \brief Request a new chunk of audio samples from the stream source
    ///
    /// This function must be overridden by derived classes to provide
    /// the audio samples to play. It is called continuously by the
    /// streaming loop, in a separate thread.
    /// The source can choose to stop the streaming loop at any time, by
    /// returning false to the caller.
    /// If you return true (i.e. continue streaming) it is important that
    /// the returned array of samples is not empty; this would stop the stream
    /// due to an internal limitation.
    ///
    /// \param data Chunk of data to fill
    ///
    /// \return True to continue playback, false to stop
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] virtual bool onGetData(Chunk& data) = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Change the current playing position in the stream source
    ///
    /// This function must be overridden by derived classes to
    /// allow random seeking into the stream source.
    ///
    /// \param timeOffset New playing position, relative to the beginning of the stream
    ///
    ////////////////////////////////////////////////////////////
    virtual void onSeek(Time timeOffset) = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Change the current playing position in the stream source to the beginning of the loop
    ///
    /// This function can be overridden by derived classes to
    /// allow implementation of custom loop points. Otherwise,
    /// it just calls onSeek(Time::Zero) and returns 0.
    ///
    /// \return The seek position after looping (or -1 if there's no loop)
    ///
    ////////////////////////////////////////////////////////////
    virtual std::int64_t onLoop();

    ////////////////////////////////////////////////////////////
    /// \brief Set the processing interval
    ///
    /// The processing interval controls the period at which the
    /// audio buffers are filled by calls to onGetData. A smaller
    /// interval may be useful for low-latency streams. Note that
    /// the given period is only a hint and the actual period may
    /// vary. The default processing interval is 10 ms.
    ///
    /// \param interval Processing interval
    ///
    ////////////////////////////////////////////////////////////
    void setProcessingInterval(Time interval);

private:
    ////////////////////////////////////////////////////////////
    /// \brief Function called as the entry point of the thread
    ///
    /// This function starts the streaming loop, and returns
    /// only when the sound is stopped.
    ///
    ////////////////////////////////////////////////////////////
    void streamData();

    ////////////////////////////////////////////////////////////
    /// \brief Fill a new buffer with audio samples, and append
    ///        it to the playing queue
    ///
    /// This function is called as soon as a buffer has been fully
    /// consumed; it fills it again and inserts it back into the
    /// playing queue.
    ///
    /// \param bufferNum Number of the buffer to fill (in [0, BufferCount])
    /// \param immediateLoop Treat empty buffers as spent, and act on loops immediately
    ///
    /// \return True if the stream source has requested to stop, false otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool fillAndPushBuffer(unsigned int bufferNum, bool immediateLoop = false);

    ////////////////////////////////////////////////////////////
    /// \brief Fill the audio buffers and put them all into the playing queue
    ///
    /// This function is called when playing starts and the
    /// playing queue is empty.
    ///
    /// \return True if the derived class has requested to stop, false otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool fillQueue();

    ////////////////////////////////////////////////////////////
    /// \brief Clear all the audio buffers and empty the playing queue
    ///
    /// This function is called when the stream is stopped.
    ///
    ////////////////////////////////////////////////////////////
    void clearQueue();

    ////////////////////////////////////////////////////////////
    /// \brief Launch a new stream thread running 'streamData'
    ///
    /// This function is called when the stream is played or
    /// when the playing offset is changed.
    ///
    ////////////////////////////////////////////////////////////
    void launchStreamingThread(Status threadStartState);

    ////////////////////////////////////////////////////////////
    /// \brief Stop streaming and wait for 'm_thread' to join
    ///
    /// This function is called when the playback is stopped or
    /// when the sound stream is destroyed.
    ///
    ////////////////////////////////////////////////////////////
    void awaitStreamingThread();

    // NOLINTBEGIN(readability-identifier-naming)
    static constexpr unsigned int BufferCount{3};   //!< Number of audio buffers used by the streaming loop
    static constexpr unsigned int BufferRetries{2}; //!< Number of retries (excluding initial try) for onGetData()
    // NOLINTEND(readability-identifier-naming)

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    std::thread                  m_thread;                    //!< Thread running the background tasks
    mutable std::recursive_mutex m_threadMutex;               //!< Thread mutex
    Status                       m_threadStartState{Stopped}; //!< State the thread starts in (Playing, Paused, Stopped)
    bool                         m_isStreaming{};             //!< Streaming state (true = playing, false = stopped)
    unsigned int                 m_buffers[BufferCount]{};    //!< Sound buffers used to store temporary audio data
    unsigned int                 m_channelCount{};            //!< Number of channels (1 = mono, 2 = stereo, ...)
    unsigned int                 m_sampleRate{};              //!< Frequency (samples / second)
    std::int32_t                 m_format{};                  //!< Format of the internal sound buffers
    bool                         m_loop{};                    //!< Loop flag (true to loop, false to play once)
    std::uint64_t                m_samplesProcessed{}; //!< Number of samples processed since beginning of the stream
    std::int64_t                 m_bufferSeeks[BufferCount]{}; //!< If buffer is an "end buffer", holds next seek position, else NoLoop. For play offset calculation.
    Time m_processingInterval{milliseconds(10)}; //!< Interval for checking and filling the internal sound buffers.
};

} // namespace sf


////////////////////////////////////////////////////////////
/// \class sf::SoundStream
/// \ingroup audio
///
/// Unlike audio buffers (see sf::SoundBuffer), audio streams
/// are never completely loaded in memory. Instead, the audio
/// data is acquired continuously while the stream is playing.
/// This behavior allows to play a sound with no loading delay,
/// and keeps the memory consumption very low.
///
/// Sound sources that need to be streamed are usually big files
/// (compressed audio musics that would eat hundreds of MB in memory)
/// or files that would take a lot of time to be received
/// (sounds played over the network).
///
/// sf::SoundStream is a base class that doesn't care about the
/// stream source, which is left to the derived class. SFML provides
/// a built-in specialization for big files (see sf::Music).
/// No network stream source is provided, but you can write your own
/// by combining this class with the network module.
///
/// A derived class has to override two virtual functions:
/// \li onGetData fills a new chunk of audio data to be played
/// \li onSeek changes the current playing position in the source
///
/// It is important to note that each SoundStream is played in its
/// own separate thread, so that the streaming loop doesn't block the
/// rest of the program. In particular, the OnGetData and OnSeek
/// virtual functions may sometimes be called from this separate thread.
/// It is important to keep this in mind, because you may have to take
/// care of synchronization issues if you share data between threads.
///
/// Usage example:
/// \code
/// class CustomStream : public sf::SoundStream
/// {
/// public:
///
///     [[nodiscard]] bool open(const std::string& location)
///     {
///         // Open the source and get audio settings
///         ...
///         unsigned int channelCount = ...;
///         unsigned int sampleRate = ...;
///
///         // Initialize the stream -- important!
///         initialize(channelCount, sampleRate);
///         return true;
///     }
///
/// private:
///
///     bool onGetData(Chunk& data) override
///     {
///         // Fill the chunk with audio data from the stream source
///         // (note: must not be empty if you want to continue playing)
///         data.samples = ...;
///
///         // Return true to continue playing
///         data.sampleCount = ...;
///         return true;
///     }
///
///     void onSeek(sf::Time timeOffset) override
///     {
///         // Change the current position in the stream source
///         ...
///     }
/// };
///
/// // Usage
/// CustomStream stream;
/// stream.open("path/to/stream");
/// stream.play();
/// \endcode
///
/// \see sf::Music
///
////////////////////////////////////////////////////////////
