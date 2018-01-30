# Audio effect config
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/audio/audio_effects.xml:system/vendor/etc/audio_effects.xml

# Audio
BOARD_USES_ALSA_AUDIO := true
AUDIO_FEATURE_DISABLED_USBAUDIO := true
AUDIO_FEATURE_ENABLED_EXTN_POST_PROC := true
AUDIO_FEATURE_ENABLED_MULTI_VOICE_SESSIONS := true

# Do not copy packages/apps/DSPManager/cyanogen-dsp/audio_policy.conf file
TARGET_USE_DEVICE_AUDIO_EFFECTS_CONF := true

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

# Hal
PRODUCT_PACKAGES += \
    audio.a2dp.default \
    audio.primary.msm8974 \
    audio.r_submix.default \
    audio.usb.default \
    audio_policy.msm8974

# SoundFX
PRODUCT_PACKAGES += \
    libaudio-resampler \
    libqcompostprocbundle \
    libqcomvisualizer \
    libqcomvoiceprocessing \
    tinymix

