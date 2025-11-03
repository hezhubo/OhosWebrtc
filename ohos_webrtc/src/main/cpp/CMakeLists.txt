# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
project(ohosWebrtc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti")
add_definitions(
    -DNODE_ADDON_API_DISABLE_DEPRECATED
    -DNAPI_DISABLE_CPP_EXCEPTIONS
    -DNDK_HELPER_DISABLE_CPP_EXCEPTIONS
    -DNODE_ADDON_API_ENABLE_TYPE_CHECK_ON_AS
)

set(OHOS_WEBRTC_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/ohos_webrtc)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/node-addon-api
    ${OHOS_WEBRTC_SRC_PATH}
)

set(LIBOHOS_WEBRTC_SOURCES
    ${OHOS_WEBRTC_SRC_PATH}/audio_processing_factory.cpp
    ${OHOS_WEBRTC_SRC_PATH}/certificate.cpp
    ${OHOS_WEBRTC_SRC_PATH}/configuration.cpp
    ${OHOS_WEBRTC_SRC_PATH}/data_channel.cpp
    ${OHOS_WEBRTC_SRC_PATH}/dtls_transport.cpp
    ${OHOS_WEBRTC_SRC_PATH}/dtmf_sender.cpp
    ${OHOS_WEBRTC_SRC_PATH}/ice_candidate.cpp
    ${OHOS_WEBRTC_SRC_PATH}/ice_transport.cpp
    ${OHOS_WEBRTC_SRC_PATH}/media_devices.cpp
    ${OHOS_WEBRTC_SRC_PATH}/media_source.cpp
    ${OHOS_WEBRTC_SRC_PATH}/media_stream.cpp
    ${OHOS_WEBRTC_SRC_PATH}/media_stream_track.cpp
    ${OHOS_WEBRTC_SRC_PATH}/media_track_constraints.cpp
    ${OHOS_WEBRTC_SRC_PATH}/napi_module.cpp
    ${OHOS_WEBRTC_SRC_PATH}/peer_connection.cpp
    ${OHOS_WEBRTC_SRC_PATH}/peer_connection_factory.cpp
    ${OHOS_WEBRTC_SRC_PATH}/rtp_parameters.cpp
    ${OHOS_WEBRTC_SRC_PATH}/rtp_receiver.cpp
    ${OHOS_WEBRTC_SRC_PATH}/rtp_sender.cpp
    ${OHOS_WEBRTC_SRC_PATH}/rtp_transceiver.cpp
    ${OHOS_WEBRTC_SRC_PATH}/sctp_transport.cpp
    ${OHOS_WEBRTC_SRC_PATH}/session_description.cpp
    ${OHOS_WEBRTC_SRC_PATH}/video_decoder_factory.cpp
    ${OHOS_WEBRTC_SRC_PATH}/video_encoder_factory.cpp
    ${OHOS_WEBRTC_SRC_PATH}/async_work/async_worker_enumerate_devices.cpp
    ${OHOS_WEBRTC_SRC_PATH}/async_work/async_worker_get_display_media.cpp
    ${OHOS_WEBRTC_SRC_PATH}/async_work/async_worker_get_stats.cpp
    ${OHOS_WEBRTC_SRC_PATH}/async_work/async_worker_get_user_media.cpp
    ${OHOS_WEBRTC_SRC_PATH}/audio_device/audio_capturer.cpp
    ${OHOS_WEBRTC_SRC_PATH}/audio_device/audio_device_enumerator.cpp
    ${OHOS_WEBRTC_SRC_PATH}/audio_device/audio_renderer.cpp
    ${OHOS_WEBRTC_SRC_PATH}/audio_device/mixing_audio_input.cpp
    ${OHOS_WEBRTC_SRC_PATH}/audio_device/ohos_audio_device_module.cpp
    ${OHOS_WEBRTC_SRC_PATH}/audio_device/ohos_local_audio_source.cpp
    ${OHOS_WEBRTC_SRC_PATH}/camera/camera_capturer.cpp
    ${OHOS_WEBRTC_SRC_PATH}/camera/camera_device_info.cpp
    ${OHOS_WEBRTC_SRC_PATH}/camera/camera_enumerator.cpp
    ${OHOS_WEBRTC_SRC_PATH}/helper/camera.cpp
    ${OHOS_WEBRTC_SRC_PATH}/helper/display_manager.cpp
    ${OHOS_WEBRTC_SRC_PATH}/logging/hilog_sink.cpp
    ${OHOS_WEBRTC_SRC_PATH}/logging/log_sink.cpp
    ${OHOS_WEBRTC_SRC_PATH}/logging/native_logging.cpp
    ${OHOS_WEBRTC_SRC_PATH}/render/egl_context.cpp
    ${OHOS_WEBRTC_SRC_PATH}/render/egl_env.cpp
    ${OHOS_WEBRTC_SRC_PATH}/render/gl_drawer.cpp
    ${OHOS_WEBRTC_SRC_PATH}/render/gl_shader.cpp
    ${OHOS_WEBRTC_SRC_PATH}/render/matrix.cpp
    ${OHOS_WEBRTC_SRC_PATH}/render/native_video_renderer.cpp
    ${OHOS_WEBRTC_SRC_PATH}/render/native_window_renderer.cpp
    ${OHOS_WEBRTC_SRC_PATH}/render/native_window_renderer_gl.cpp
    ${OHOS_WEBRTC_SRC_PATH}/render/native_window_renderer_raster.cpp
    ${OHOS_WEBRTC_SRC_PATH}/render/video_frame_drawer.cpp
    ${OHOS_WEBRTC_SRC_PATH}/render/yuv_converter.cpp
    ${OHOS_WEBRTC_SRC_PATH}/screen_capture/screen_capture_options.cpp
    ${OHOS_WEBRTC_SRC_PATH}/screen_capture/screen_capturer.cpp
    ${OHOS_WEBRTC_SRC_PATH}/screen_capture/system_audio_receiver.cpp
    ${OHOS_WEBRTC_SRC_PATH}/user_media/media_constraints.cpp
    ${OHOS_WEBRTC_SRC_PATH}/user_media/media_constraints_util.cpp
    ${OHOS_WEBRTC_SRC_PATH}/video/texture_buffer.cpp
    ${OHOS_WEBRTC_SRC_PATH}/video/video_frame_receiver_gl.cpp
    ${OHOS_WEBRTC_SRC_PATH}/video/video_frame_receiver_native.cpp
    ${OHOS_WEBRTC_SRC_PATH}/video/video_track_source.cpp
    ${OHOS_WEBRTC_SRC_PATH}/video_codec/default_video_decoder_factory.cpp
    ${OHOS_WEBRTC_SRC_PATH}/video_codec/default_video_encoder_factory.cpp
    ${OHOS_WEBRTC_SRC_PATH}/video_codec/hardware_video_decoder.cpp
    ${OHOS_WEBRTC_SRC_PATH}/video_codec/hardware_video_decoder_factory.cpp
    ${OHOS_WEBRTC_SRC_PATH}/video_codec/hardware_video_encoder.cpp
    ${OHOS_WEBRTC_SRC_PATH}/video_codec/hardware_video_encoder_factory.cpp
    ${OHOS_WEBRTC_SRC_PATH}/video_codec/media_codec_utils.cpp
    ${OHOS_WEBRTC_SRC_PATH}/video_codec/software_video_decoder_factory.cpp
    ${OHOS_WEBRTC_SRC_PATH}/video_codec/software_video_encoder_factory.cpp
)

add_library(ohos_webrtc SHARED ${LIBOHOS_WEBRTC_SOURCES})

target_link_libraries(ohos_webrtc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/webrtc/libs/${OHOS_ARCH}/libwebrtc.a
    libace_napi.z.so
    libhilog_ndk.z.so
    libohaudio.so
    libohcamera.so
    libohimage.so
    libimage_receiver.so
    libnative_buffer.so
    libnative_window.so
    libnative_image.so
    libEGL.so
    libGLESv3.so
    libnative_media_codecbase.so
    libnative_media_core.so
    libnative_media_venc.so
    libnative_media_vdec.so
    libnative_avscreen_capture.so
    libnative_drawing.so
    libnative_display_manager.so
)

set(WEBRTC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/deps/webrtc/include)
target_compile_definitions(ohos_webrtc PUBLIC -DWEBRTC_POSIX)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH})
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/abseil-cpp)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/boringssl)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/catapult)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/crc32c)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/dav1d)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/ffmpeg)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/fontconfig)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/fuzztest)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/google_benchmark)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/googletest)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/harfbuzz-ng)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/icu)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/jsoncpp)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/libaom)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/catapult)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/libc++)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/libc++abi)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/libevent)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/libjpeg_turbo)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/libpng)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/libsrtp)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/libvpx)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/libyuv/include)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/llvm-build)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/nasm)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/openh264)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/opus)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/pffft)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/protobuf)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/re2)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/rnnoise)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/zlib)
target_include_directories(ohos_webrtc PUBLIC ${WEBRTC_PATH}/third_party/jsoncpp/source/include)