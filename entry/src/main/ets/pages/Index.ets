/**
 * Copyright (c) 2024 Archermind Technology (Nanjing) Co. Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import webrtc, { Logging, LoggingSeverity, VideoRenderController, ScalingMode } from '@ohos/webrtc';
import abilityAccessCtrl, { PermissionRequestResult } from '@ohos.abilityAccessCtrl';
import webSocket from '@ohos.net.webSocket';
import promptAction from '@ohos.promptAction';
import buffer from '@ohos.buffer';
import fs from '@ohos.file.fs';
import audio from '@ohos.multimedia.audio';
import { BusinessError } from '@kit.BasicServicesKit';

const TAG: string = '[Index]';

// 用于测试的信令服务器
const SIGNAL_SERVER = 'wss://youzhi.life:8443';
// 用于测试的Web客户端地址
// https://webrtc.youzhi.life/
// 用于测试的STUN服务器地址
const STUN_SERVER = "stun:stun.l.google.com:19302"

interface MessageData {
  sdp?: webrtc.RTCSessionDescriptionInit;
  ice?: webrtc.RTCIceCandidateInit;
}

@Entry
@Component
struct Index {
  @State buttonTitle: string = 'Connect';
  @State statusText: string = '';
  @State private myId: string = '';
  @State private peerId: string = '';
  @State muteBtnTitle: string = 'Mute';
  private muted: boolean = false;
  private mXComponentController1: VideoRenderController = new VideoRenderController();
  private mXComponentController2: VideoRenderController = new VideoRenderController();
  private ws: webSocket.WebSocket = webSocket.createWebSocket();
  private localAudioSource?: webrtc.AudioSource;
  private localAudioSource2?: webrtc.AudioSource;
  private localVideoSource?: webrtc.VideoSource;
  private localAudioTrack?: webrtc.MediaStreamTrack;
  private localAudioTrack2?: webrtc.MediaStreamTrack;
  private localVideoTrack?: webrtc.MediaStreamTrack;
  private remoteAudioTrack?: webrtc.MediaStreamTrack;
  private remoteVideoTrack?: webrtc.MediaStreamTrack;
  private adm?: webrtc.AudioDeviceModule;
  private pcf?: webrtc.PeerConnectionFactory;
  private pc?: webrtc.RTCPeerConnection;
  private recvChannel?: webrtc.RTCDataChannel;
  private sendChannel?: webrtc.RTCDataChannel;
  private displayMediaStream?: webrtc.MediaStream;
  private mediaDevices = new webrtc.MediaDevices();
  private onDataChannelMessageReceived = (event: webrtc.MessageEvent<string | ArrayBuffer>) => {
    // this is Index
    console.info(TAG, "dataChannel.OnMessage: ", event);
    console.info(TAG, "dataChannel.OnMessage: (typeof event.data) = ", typeof event.data);
    console.info(TAG, "dataChannel.OnMessage: (event.data instanceof ArrayBuffer) = ", event.data instanceof ArrayBuffer);

    this.setStatus("Received data channel message");

    if (typeof event.data === 'string' || event.data instanceof String) {
      console.info(TAG, 'Incoming string message: ' + event.data);
    } else {
      console.info(TAG, 'Incoming data message');
    }
  }

  //
  build() {
    Column() {
      Row() {
        Text("状态")
          .fontSize(20)
          .fontWeight(FontWeight.Normal)
          .margin({ right: 24 })
        Text(this.statusText)
          .fontSize(20)
          .fontWeight(FontWeight.Bold)
      }
      .alignSelf(ItemAlign.Start)

      Row() {
        Text("本机ID")
          .fontSize(20)
          .fontWeight(FontWeight.Normal)
          .margin({ right: 24 })
          .align(Alignment.Start)
        Text(this.myId)
          .fontSize(20)
          .fontWeight(FontWeight.Bold)
      }
      .alignSelf(ItemAlign.Start)
      .padding({ top: 10 })

      Row() {
        TextInput({ placeholder: '请输入对端ID' })
          .width('100%')
          .height('auto')
          .fontSize(18)
          .fontWeight(FontWeight.Normal)
          .type(InputType.Number)
          .onChange((value: string) => {
            this.peerId = value;
          })
      }
      .alignSelf(ItemAlign.Start)
      .padding({ top: 24 })

      Row() {
        Button(this.buttonTitle)
          .width('auto')
          .height('auto')
          .fontSize(15)
          .fontWeight(FontWeight.Bold)
          .margin({ top: 20 })
          .onClick(() => {
            if (this.buttonTitle == 'Disconnect') {
              this.ws.close();
              this.buttonTitle = 'Connect';
              return;
            }

            if (this.peerId.length < 2 || this.peerId.length > 4) {
              promptAction.showToast({ message: "请输入2到4位数ID" });
              return;
            }

            this.sendMessage('SESSION ' + this.peerId);
            this.buttonTitle = 'Disconnect';
          })
          .margin({ left: 10, right: 20 })

        Row() {
          Checkbox({ name: 'remote_offerer', group: 'checkboxGroup' })
            .select(false)
            .selectedColor(0x39a2db)
            .onChange((value: boolean) => {
              console.info('remote_offerer change is' + value)

            }).margin({ right: 8 })

          Text("Remote offerer")
            .fontSize(15)
            .fontWeight(FontWeight.Normal)
        }.visibility(Visibility.Hidden)

        Button(this.muteBtnTitle)
          .width('auto')
          .height('auto')
          .fontSize(15)
          .fontWeight(FontWeight.Bold)
          .margin({ top: 20 })
          .onClick(() => {
            console.log(TAG, "audio muted: " + this.muted);
            this.muted = !this.muted;
            this.muteBtnTitle = this.muted ? 'Unmute' : 'Mute';
            this.pc?.setAudioPlayout(!this.muted);
            this.pc?.setAudioRecording(!this.muted);
          })
          .margin({ left: 10, right: 20 })

      }
      .alignSelf(ItemAlign.Start)
      .padding({ top: 24 })

      Row() {
        XComponent({
          id: "local-video",
          type: XComponentType.SURFACE,
          controller: this.mXComponentController1
        })
          .width('480px')
          .height('480px')
          .onLoad((context?) => {
            console.log(TAG, "context: " + context);
            console.log(TAG, "surfaceId: " + this.mXComponentController1.getXComponentSurfaceId());
          })
          .onDestroy(() => {
            console.log(TAG, "onDestroy");
          })

        XComponent({
          id: "remote-video",
          type: XComponentType.SURFACE,
          controller: this.mXComponentController2
        })
          .width('480px')
          .height('480px')
          .onLoad((context?) => {
            console.log(TAG, "context: " + JSON.stringify(context));
            console.log(TAG, "surfaceId: " + this.mXComponentController2.getXComponentSurfaceId());
          })
          .onDestroy(() => {
            console.log(TAG, "onDestroy");
          })
      }
      .alignSelf(ItemAlign.Start)
      .padding({ top: 10 })

    }
    .width('100%')
    .height('100%')
    .margin({ left: 24, right: 24, top: 24, bottom: 24 })
    .justifyContent(FlexAlign.Start)
  }

  aboutToAppear(): void {
    // 设置音频策略，允许和其它音频流并发
    let strategy: audio.AudioSessionStrategy = {
      concurrencyMode: audio.AudioConcurrencyMode.CONCURRENCY_MIX_WITH_OTHERS
    };

    let audioManager = audio.getAudioManager();
    let audioSessionManager = audioManager.getSessionManager();
    audioSessionManager.activateAudioSession(strategy).then(() => {
      console.info(TAG, 'activateAudioSession SUCCESS');
    }).catch((err: BusinessError) => {
      console.error(`ERROR: ${err}`);
    });

    Logging.enableLogThreads();
    Logging.enableLogTimeStamps();
    Logging.enableLogToDebugOutput(LoggingSeverity.VERBOSE);

    abilityAccessCtrl.createAtManager()
      .requestPermissionsFromUser(getContext(), ['ohos.permission.CAMERA', 'ohos.permission.MICROPHONE'])
      .then(async (result: PermissionRequestResult) => {
        let granted = true;
        for (let index = 0; index < result.permissions.length; index++) {
          console.info(TAG, "request permission [" + result.permissions[index] + "] with result: " + result.authResults[index]);
          if (result.authResults[index] != abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED) {
            granted = false;
          }
        }

        if (!granted) {
          this.setError("Permission Not Granted");
          return;
        }

        console.info(TAG, "Connecting to server");
        this.setStatus("Connecting to server");
        this.ws.connect(SIGNAL_SERVER, (err, success) => {
        })
      });

    this.ws.on('open', (err, data) => {
      console.info(TAG, 'websocket open');
      this.myId = this.getRandomId();
      this.sendMessage("HELLO " + this.myId);
      this.setStatus("Registering with server");
    });

    this.ws.on('close', (err, data: webSocket.CloseResult) => {
      console.info(TAG, 'websocket closed');
      this.setStatus('Disconnected from server');

      this.pc?.close();
      this.pc = undefined;

      setTimeout(() => {
        this.ws.connect(SIGNAL_SERVER);
      }, 1000);
    });

    this.ws.on('error', (err) => {
      console.info(TAG, 'websocket err: ' + err.code);
      this.setError("Unable to connect to server: " + err.message);

      setTimeout(() => {
        this.ws.connect(SIGNAL_SERVER);
      }, 1000);
    });

    this.ws.on('message', (err, data: string | ArrayBuffer) => {
      console.info(TAG, 'websocket receive message: ' + data);
      if (data === 'HELLO') {
        this.setStatus("Registered with server, waiting for call");
        this.createPeerConnection();
      } else if (data === 'SESSION_OK') {
        this.setStatus("Starting negotiation");
        this.sendOffer();
      } else if (data === 'OFFER_REQUEST') {
        this.sendOffer();
      } else {
        if (data.toString().startsWith('ERROR')) {
          this.handleIncomingError(data.toString());
        } else {
          try {
            let msg = JSON.parse(data.toString()) as MessageData;
            if (msg && msg.sdp) {
              this.receiveSdp(msg.sdp);
            } else if (msg && msg.ice) {
              this.receiveIce(msg.ice);
            } else {
              console.error(TAG, 'receive unknown message: ' + data);
              this.handleIncomingError("Unknown incoming JSON");
            }
          } catch (error) {
            console.error(TAG, 'receive invalid message: ' + data);
          }
        }
      }
    });

    this.createAudioDeviceModule();
    this.createPeerConnectionFactory();
  }

  aboutToDisappear(): void {
    this.pcf?.stopAecDump();
    Logging.deleteInjectedLoggable();

    this.ws.close();

    this.ws.off('open');
    this.ws.off('close');
    this.ws.off('error');
    this.ws.off('message');
  }

  onPageShow(): void {
    console.info(TAG, 'onPageShow');
  }

  onPageHide(): void {
    console.info(TAG, 'onPageHide');
  }

  getRandomId() {
    return Math.floor(Math.random() * (9000 - 10) + 10).toString();
  }

  setStatus(text: string) {
    console.info(TAG, text);
    this.statusText = text;
  }

  setError(error: string) {
    console.error(TAG, error);
    this.statusText = error;
  }

  handleIncomingError(error: string) {
    this.setError('ERROR: ' + error);
    this.ws.close();
  }

  async createAudioDeviceModule() {
    this.adm = new webrtc.AudioDeviceModule({ useStereoInput: true, useStereoOutput: true });
    this.adm.oncapturererror = (event) => {
      console.error(TAG, 'oncapturererror: ' + JSON.stringify(event));
    };
    this.adm.oncapturerstatechange = (event) => {
      console.info(TAG, 'oncapturerstatechange: ' + JSON.stringify(event));
    };
    this.adm.oncapturersamplesready = (event) => {
      console.debug(TAG, 'oncapturersamplesready: ' + JSON.stringify(event));
    };
    this.adm.onrenderererror = (event) => {
      console.error(TAG, 'onrenderererror: ' + JSON.stringify(event));
    };
    this.adm.onrendererstatechange = (event) => {
      console.info(TAG, 'onrendererstatechange: ' + JSON.stringify(event));
    };
    Logging.d(TAG, 'this.adm: ' + JSON.stringify(this.adm));
  }

  async createPeerConnectionFactory() {
    this.pcf = new webrtc.PeerConnectionFactory({
      videoEncoderFactory: new webrtc.HardwareVideoEncoderFactory,
      videoDecoderFactory: new webrtc.HardwareVideoDecoderFactory,
      adm: this.adm
    });
    Logging.d(TAG, 'this.pcf: ' + JSON.stringify(this.pcf));

    // MediaDevices 依赖默认的 PeerConnnectionFactory，且需要与创建 PeerConnection 的是同一个。
    webrtc.PeerConnectionFactory.setDefault(this.pcf);

    let filesDir = getContext().filesDir;
    let file = fs.openSync(filesDir + '/test.txt', fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE);
    this.pcf.startAecDump(file.fd, -1);
  }

  async prepareLocalMedia() {
    if (!this.localAudioTrack) {
      this.localAudioSource = this.pcf?.createAudioSource({ echoCancellation: true, noiseSuppression: true });
      console.info(TAG, 'audioSource: ' + JSON.stringify(this.localAudioSource));

      this.localAudioTrack = this.pcf?.createAudioTrack("audio", this.localAudioSource);
      console.info(TAG, 'audioTrack: ' + JSON.stringify(this.localAudioTrack));
    }

    if (!this.localVideoTrack) {
      this.localVideoSource = this.pcf?.createVideoSource({width: 1280, height: 720, facingMode: 'user'}, false);
      console.info(TAG, 'videoSource: ' + JSON.stringify(this.localVideoSource));
      this.localVideoSource!.oncapturerstarted = (event: webrtc.VideoCapturerStartedEvent) => {
        console.info(TAG, 'oncapturerstarted: event=' + JSON.stringify(event));
      };
      this.localVideoSource!.oncapturerstopped = (event: webrtc.Event) => {
        console.info(TAG, 'oncapturerstopped: event=' + JSON.stringify(event));
      };

      this.localVideoTrack = this.pcf?.createVideoTrack("video", this.localVideoSource);
      console.info(TAG, 'videoTrack: ' + JSON.stringify(this.localVideoTrack));

      if (this.localVideoTrack) {
        this.mXComponentController1.setVideoTrack(this.localVideoTrack);
      }

      this.mXComponentController1.setMirror(true);
      this.mXComponentController1.setScalingMode(ScalingMode.AspectFit);
    }
  }

  async prepareDisplayMedia() {
    if (!this.displayMediaStream) {
      this.displayMediaStream = await this.mediaDevices.getDisplayMedia({
        video: {
          width: 720,
          height: 1280,
          ohosScreenCaptureAudioFilter: 'current-app', // 过滤当前应用播放的音频
        },
        audio: true,
        ohosSystemAudio: true
      });

      this.displayMediaStream.getTracks().forEach((track) => console.info(TAG, 'display media Stream track: ' + JSON.stringify(track)));

      this.localVideoTrack = this.displayMediaStream.getVideoTracks()[0];
      console.info(TAG, 'videoTrack: ' + JSON.stringify(this.localVideoTrack));

      if (this.localVideoTrack) {
        let videoSource = this.localVideoTrack.getSource();
        if (videoSource) {
          this.localVideoSource = videoSource as webrtc.VideoSource;
          console.info(TAG, 'videoSource: ' + JSON.stringify(this.localVideoSource));
          this.localVideoSource!.oncapturerstarted = (event: webrtc.VideoCapturerStartedEvent) => {
            console.info(TAG, 'oncapturerstarted: event=' + JSON.stringify(event));
          };
          this.localVideoSource!.oncapturerstopped = (event: webrtc.Event) => {
            console.info(TAG, 'oncapturerstopped: event=' + JSON.stringify(event));
          };
        }

        this.mXComponentController1.setVideoTrack(this.localVideoTrack);
        this.mXComponentController1.setScalingMode(ScalingMode.AspectFit);
      }

      let audioTracks = this.displayMediaStream.getAudioTracks();
      if (audioTracks.length > 0) {
        this.localAudioTrack = audioTracks[0];
        console.info(TAG, 'audioTrack: ' + JSON.stringify(this.localAudioTrack));
      }
      if (audioTracks.length > 1) {
        this.localAudioTrack2 = audioTracks[1];
        console.info(TAG, 'audioTrack2: ' + JSON.stringify(this.localAudioTrack2));
      }
    }
  }

  async createPeerConnection() {
    // await this.prepareLocalMedia();
    await this.prepareDisplayMedia();

    this.pc = this.pcf?.createPeerConnection({
      iceServers: [{ urls: STUN_SERVER }]
    });

    if (!this.pc) {
      console.error(TAG, 'failed to create RTCPeerConnection');
      return;
    }

    this.pc.ontrack = (event: webrtc.RTCTrackEvent) => {
      console.info(TAG, 'ontrack: ' + JSON.stringify(event));
      if (event.track.kind == 'video') {
        this.remoteVideoTrack = event.track;
        this.mXComponentController2.setVideoTrack(event.track);
      } else if (event.track.kind == 'audio') {
        this.remoteAudioTrack = event.track;
      }
    };
    this.pc.onsignalingstatechange = (event) => {
      console.info(TAG, 'onsignalingstatechange: ' + this.pc?.signalingState);
    }
    this.pc.onconnectionstatechange = (event) => {
      console.info(TAG, 'onconnectionstatechange: ' + this.pc?.connectionState);
      if (this.pc?.connectionState == 'failed') {
        this.setError("Peer connection failed");
        this.ws.close();
      }
    }
    this.pc.onnegotiationneeded = (event) => {
      // this is Index
      console.info(TAG, 'onnegotiationneeded: this=' + JSON.stringify(this.pc));
      console.info(TAG, 'onnegotiationneeded: event=' + JSON.stringify(event));
    }

    this.pc.onicecandidate = (event) => {
      console.info(TAG, 'onicecandidate: this=' + JSON.stringify(this));
      console.info(TAG, 'onicecandidate: event=' + JSON.stringify(event));
      if (event.candidate) {
        this.sendMessage(JSON.stringify({ ice: event.candidate }));
      }
    };
    this.pc.onicecandidateerror = (event) => {
      console.error(TAG, 'onicecandidateerror: event=' + JSON.stringify(event));
    };
    this.pc.oniceconnectionstatechange = (event) => {
      console.info(TAG, 'oniceconnectionstatechange: ' + this.pc?.iceConnectionState);
      if (this.pc?.iceConnectionState === 'failed') {
        this.setError("Ice connection failed");
        this.ws.close();
      }
    };
    this.pc.onicegatheringstatechange = (event) => {
      console.info(TAG, 'onicegatheringstatechange: ' + this.pc?.iceGatheringState);
      switch (this.pc?.iceGatheringState) {
        case "new":
          console.info(TAG, 'onicegatheringstatechange: gathering is either just starting or has been reset');
          break;
        case "gathering":
          console.info(TAG, 'onicegatheringstatechange: gathering has begun or is ongoing');
          break;
        case "complete":
          console.info(TAG, 'onicegatheringstatechange: gathering has ended');
          break;
      }
    };
    this.pc.ondatachannel = (event) => {
      console.info(TAG, 'ondatachannel: this=' + JSON.stringify(this));
      console.info(TAG, 'ondatachannel: event=' + JSON.stringify(event));
      this.setStatus("Data channel created");
      this.recvChannel = event.channel;
      this.recvChannel.onopen = this.onDataChannelOpen;
      this.recvChannel.onmessage = this.onDataChannelMessageReceived;
      this.recvChannel.onerror = this.onDataChannelError;
      this.recvChannel.onclose = this.onDataChannelClose;
    }

    if (this.localAudioTrack) {
      let rtpSender1 = this.pc.addTrack(this.localAudioTrack);
      console.info(TAG, 'rtpSender1: ' + JSON.stringify(rtpSender1));
    }
    if (this.localAudioTrack2) {
      let rtpSender2 = this.pc.addTrack(this.localAudioTrack2);
      console.info(TAG, 'rtpSender2: ' + JSON.stringify(rtpSender2));
    }
    if (this.localVideoTrack) {
      let rtpSender3 = this.pc.addTrack(this.localVideoTrack);
      console.info(TAG, 'rtpSender3: ' + JSON.stringify(rtpSender3));
    }

    console.info(TAG, 'transceivers: ' + JSON.stringify(this.pc.getTransceivers()));

    this.sendChannel = this.pc.createDataChannel("send");
    console.info(TAG, 'sendChannel: ' + JSON.stringify(this.sendChannel));
    this.sendChannel.onopen = (event) => {
      // this is Index
      console.info(TAG, '=dataChannel.OnOpen: this=', JSON.stringify(this));
      console.info(TAG, "=dataChannel.OnOpen: ", event);

      this.sendChannel?.send("Hi! (from ArkTS)");
      let data1 = buffer.alloc(20, 0);
      this.sendChannel?.send(data1.buffer);

      let data2 = new Int32Array(new ArrayBuffer(20));
      data2.fill(1);
      this.sendChannel?.send(data2.buffer);

      let data3 = new Uint8Array(20);
      data3.fill(2);
      this.sendChannel?.send(data3.buffer);
    };
    this.sendChannel.onmessage = this.onDataChannelMessageReceived;
    this.sendChannel.onerror = this.onDataChannelError;
    this.sendChannel.onclose = this.onDataChannelClose;
  }

  onDataChannelOpen(event: webrtc.Event) {
    // this is webrtc.RTCDataChannel
    console.info(TAG, "dataChannel.OnOpen: ", event);
    if (this instanceof webrtc.RTCDataChannel) {
      this.send("Hi! (from ArkTS)");
    }
  }

  async onDataChannelError(event: webrtc.Event) {
    // this is webrtc.RTCDataChannel
    console.info(TAG, "dataChannel.OnError: ", event);
  }

  async onDataChannelClose(event: webrtc.Event) {
    // this is webrtc.RTCDataChannel
    console.info(TAG, 'dataChannel.OnClose: this=', JSON.stringify(this));
    console.info(TAG, "dataChannel.OnClose", event);
  }

  async sendOffer() {
    try {
      let offer = await this.pc?.createOffer();
      console.info(TAG, 'createOffer: ' + JSON.stringify(offer));

      this.pc?.setLocalDescription(offer);

      this.sendMessage(JSON.stringify({ sdp: offer }));
    } catch (e) {
      console.error(TAG, 'sendOffer: ' + JSON.stringify(e));
    }
  }

  async sendAnswer() {
    this.setStatus("Got SDP offer, creating answer");

    this.pc?.createAnswer().then((desc) => {
      console.info(TAG, 'Success to create answer: ' + JSON.stringify(desc));
      this.pc?.setLocalDescription(desc).then(() => {
        this.setStatus("Sending SDP answer");
        this.sendMessage(JSON.stringify({ sdp: this.pc?.localDescription }));
      });
    }).catch((e: Error) => {
      console.error(TAG, 'Failed to create answer: ' + e.message);
      this.setError(e.message);
    });
  }

  async sendMessage(message: string) {
    console.info(TAG, 'websocket send: ' + message);
    this.ws.send(message, (error, success) => {
      if (success) console.info(TAG, 'websocket send success: ' + message);
      else console.error(TAG, 'websocket send error: ' + error);
    });
  }

  async receiveSdp(sdp: webrtc.RTCSessionDescriptionInit) {
    if (sdp.type === 'offer') {
      console.info(TAG, 'receive offer');
      this.pc?.setRemoteDescription(sdp).then(() => {
        this.setStatus("Remote SDP set");
        this.sendAnswer();
      }).catch((e: Error) => {
        console.error(TAG, "Set remote description error: " + e);
        this.setError(e.message);
      });
    } else if (sdp.type === 'answer') {
      console.info(TAG, 'receive answer');
      this.pc?.setRemoteDescription(sdp).then(() => {
        this.setStatus("Remote SDP set");
      }).catch((e: Error) => {
        console.error(TAG, "Set remote description error: " + e);
        this.setError(e.message);
      });
    }
  }

  async receiveIce(ice: webrtc.RTCIceCandidateInit) {
    console.info(TAG, 'receive icecandidate');
    this.pc?.addIceCandidate(ice).catch((e: Error) => {
      console.error(TAG, "add ice candidate error: " + e);
      this.setError(e.message);
    });
  }
}
