#pragma once

#include "karma/audio/backend.hpp"

#include <memory>
#include <string>

namespace karma::audio {

class AudioSystem {
 public:
    void setBackend(audio::backend::BackendKind backend) { requested_backend_ = backend; }
    audio::backend::BackendKind requestedBackend() const { return requested_backend_; }
    audio::backend::BackendKind selectedBackend() const { return selected_backend_; }
    const char* selectedBackendName() const;
    bool isInitialized() const { return initialized_; }

    void init();
    void shutdown();
    void beginFrame(float dt);
    void update(float dt);
    void endFrame();

    void setListener(const audio::backend::ListenerState& state);
    void playOneShot(const audio::backend::PlayRequest& request);
    audio::backend::VoiceId startVoice(const audio::backend::PlayRequest& request);
    void stopVoice(audio::backend::VoiceId voice);

 private:
    audio::backend::BackendKind requested_backend_ = audio::backend::BackendKind::Auto;
    audio::backend::BackendKind selected_backend_ = audio::backend::BackendKind::Auto;
    std::unique_ptr<audio::backend::Backend> backend_{};
    bool initialized_ = false;
};

} // namespace karma::audio

