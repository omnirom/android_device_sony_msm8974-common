# Snapdragon audio processing
PRODUCT_PROPERTY_OVERRIDES += \
    persist.speaker.prot.enable=false \
    audio.deep_buffer.media=true \
    af.fast_track_multiplier=1 \
    audio_hal.period_size=192 \
    audio.offload.pcm.16bit.enable=true \
    audio.offload.pcm.24bit.enable=true \
    audio.offload.buffer.size.kb=32 \
    audio.offload.video=false \
    audio.offload.gapless.enabled=false \
    av.streaming.offload.enable=enable \
    media.aac_51_output_enabled=true \
    qcom.hw.aac.encoder=true

