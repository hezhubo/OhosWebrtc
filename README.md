# Open harmony OS webrtc

## Building command

```
gn gen out/webrtc_arm64 --args='is_clang=true target_cpu="arm64" target_os="ohos" ohos_sdk_native_root="/home/hezb/huawei/command-line-tools/sdk/default/openharmony/native/" is_debug=true rtc_use_h264=true rtc_enable_protobuf=false use_rtti=true use_custom_libcxx=false is_component_build=false treat_warnings_as_errors=false rtc_include_tests=false'

ninja -C out/webrtc_arm64 -v -j32
```

// TODO remove node-addon-api

----
Referenced open-source projects

* [ohos_webrtc](https://gitcode.com/openharmony-sig/ohos_webrtc)