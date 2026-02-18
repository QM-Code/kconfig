#include "audio/backends/factory_internal.hpp"

#include "karma/common/logging/logging.hpp"

#if !defined(KARMA_HAS_AUDIO_SDL3AUDIO)

namespace karma::audio::backend {
namespace {

class Sdl3AudioBackendStub final : public Backend {
 public:
    const char* name() const override { return "sdl3audio"; }

    bool init() override {
        KARMA_TRACE("audio.sdl3audio", "AudioBackend[sdl3audio]: unavailable (not compiled)");
        return false;
    }

    void shutdown() override {}
    void beginFrame(float) override {}
    void update(float) override {}
    void endFrame() override {}
    void setListener(const ListenerState&) override {}
    void playOneShot(const PlayRequest&) override {}
    VoiceId startVoice(const PlayRequest&) override { return kInvalidVoiceId; }
    void stopVoice(VoiceId) override {}
};

} // namespace

std::unique_ptr<Backend> CreateSdl3AudioBackend() {
    return std::make_unique<Sdl3AudioBackendStub>();
}

} // namespace karma::audio::backend

#endif // !defined(KARMA_HAS_AUDIO_SDL3AUDIO)
