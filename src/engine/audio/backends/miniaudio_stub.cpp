#include "audio/backends/factory_internal.hpp"

#include "karma/common/logging/logging.hpp"

#if !defined(KARMA_HAS_AUDIO_MINIAUDIO)

namespace karma::audio::backend {
namespace {

class MiniaudioBackendStub final : public Backend {
 public:
    const char* name() const override { return "miniaudio"; }

    bool init() override {
        KARMA_TRACE("audio.miniaudio", "AudioBackend[miniaudio]: unavailable (not compiled)");
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

std::unique_ptr<Backend> CreateMiniaudioBackend() {
    return std::make_unique<MiniaudioBackendStub>();
}

} // namespace karma::audio::backend

#endif // !defined(KARMA_HAS_AUDIO_MINIAUDIO)
