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

export type RTCErrorDetailType = "data-channel-failure" | "dtls-failure" | "fingerprint-failure" | "hardware-encoder-error" | "hardware-encoder-not-available" | "sctp-failure" | "sdp-syntax-error";
export type RTCIceProtocol = "tcp" | "udp";
export type RTCIceCandidateType = "host" | "prflx" | "relay" | "srflx";
export type RTCIceTcpCandidateType = "active" | "passive" | "so";
export type RTCIceComponent = "rtp" | "rtcp";
export type RTCIceGathererState = "complete" | "gathering" | "new";
export type RTCIceTransportState = "checking" | "closed" | "completed" | "connected" | "disconnected" | "failed" | "new";
export type RTCIceRole = "unknown" | "controlling" | "controlled";
export type RTCSdpType = 'offer' | 'answer' | "pranswer" | "rollback";
export type BinaryType = "blob" | "arraybuffer";
export type DataChannelState = "closed" | "closing" | "connecting" | "open";
export type RTCDtlsTransportState = "new" | "connecting" | "connected" | "closed" | "failed";
export type RTCIceGatheringState = "new" | "gathering" | "complete";
export type RTCIceConnectionState = "checking" | "closed" | "completed" | "connected" | "disconnected" | "failed" | "new";
export type RTCSignalingState = "closed" | "have-local-offer" | "have-local-pranswer" | "have-remote-offer" | "have-remote-pranswer" | "stable";
export type RTCPeerConnectionState = "closed" | "connected" | "connecting" | "disconnected" | "failed" | "new";
export type RTCBundlePolicy = "balanced" | "max-bundle" | "max-compat";
export type RTCRtcpMuxPolicy = "require";
export type RTCIceTransportPolicy = "all" | "relay";
export type DegradationPreference = "balanced" | "maintain-framerate" | "maintain-resolution";
// export type RTCPriorityType = "high" | "low" | "medium" | "very-low";
export type RTCRtpTransceiverDirection = "inactive" | "recvonly" | "sendonly" | "sendrecv" | "stopped";
export type RTCSctpTransportState = "connecting" | "connected" | "closed";
export type RTCStatsType = "candidate-pair" | "certificate" | "codec" | "data-channel" | "inbound-rtp" | "local-candidate" | "media-playout" | "media-source" | "outbound-rtp" | "peer-connection" | "remote-candidate" | "remote-inbound-rtp" | "remote-outbound-rtp" | "transport";
export type RTCStatsIceCandidatePairState = "failed" | "frozen" | "in-progress" | "succeeded" | "waiting";
export type MediaStreamTrackState = 'live' | 'ended';
export type MediaDeviceKind = 'audioinput' | 'audiooutput' | 'videoinput';
export type MediaSourceState = 'initializing' | 'live' | 'ended' | 'muted';
export type VideoFacingModeEnum = "user" | "environment" | "left" | "right";
export type VideoResizeModeEnum = "none" | "crop-and-scale";
export type AudioErrorType = 'init' | 'start-exception' | 'start-state-mismatch' | 'general';
export type AudioState = 'start' | 'stop';
export type ScreenCaptureMode = 'home-screen' | 'specified-screen' | 'specified-window';
export type ScreenCaptureAudioFilterEnum = 'current-app' | 'notification';

export type AlgorithmIdentifier = Algorithm | string;
export type HighResTimeStamp = number;
export type EpochTimeStamp = number;
export type ConstrainBoolean = boolean | ConstrainBooleanParameters;
export type ConstrainULong = number | ConstrainULongRange;
export type ConstrainDouble = number | ConstrainDoubleRange;
export type ConstrainString = string | string[] | ConstrainStringParameters;

export interface ULongRange {
  max?: number;
  min?: number;
}

export interface DoubleRange {
  max?: number;
  min?: number;
}

export interface ConstrainBooleanParameters {
  exact?: boolean;
  ideal?: boolean;
}

export interface ConstrainStringParameters {
  exact?: string | string[];
  ideal?: string | string[];
}

export interface ConstrainDoubleRange extends DoubleRange {
  exact?: number;
  ideal?: number;
}

export interface ConstrainULongRange extends ULongRange {
  exact?: number;
  ideal?: number;
}

// event
// base class of events
export interface Event {
  readonly type: string;
}

// declare var Event: {
//   prototype: Event;
//   new(): Event;
// };

export interface EventTarget {
  // empty for now
}

// error
// https://www.w3.org/TR/webrtc/#rtcerrorinit-dictionary
export interface RTCErrorInit {
  errorDetail: RTCErrorDetailType;
  sdpLineNumber?: number;
  sctpCauseCode?: number;
  receivedAlert?: number;
  sentAlert?: number;
}

// https://www.w3.org/TR/webrtc/#rtcerror-interface
export interface RTCError extends /*DOMException*/ Error {
  readonly errorDetail: RTCErrorDetailType;
  readonly sdpLineNumber?: number;
  readonly sctpCauseCode?: number;
  readonly receivedAlert?: number;
  readonly sentAlert?: number;
}

// https://www.w3.org/TR/webrtc/#rtcerrorevent-interface
export interface RTCErrorEvent extends Event {
  readonly error: RTCError;
}

// https://www.w3.org/TR/webrtc/#rtctrackevent
export interface RTCTrackEvent extends Event {
  readonly receiver: RTCRtpReceiver;
  readonly track: MediaStreamTrack;
  readonly streams: ReadonlyArray<MediaStream>;
  readonly transceiver: RTCRtpTransceiver;
}

// https://www.w3.org/TR/webrtc/#rtcpeerconnectioniceevent
export interface RTCPeerConnectionIceEvent extends Event {
  readonly candidate?: RTCIceCandidate;
  readonly url?: string | null;
}

// https://www.w3.org/TR/webrtc/#rtcpeerconnectioniceerrorevent
export interface RTCPeerConnectionIceErrorEvent extends Event {
  readonly address?: string;
  readonly port?: number;
  readonly url?: string;
  readonly errorCode?: number;
  readonly errorText?: string;
}

// https://www.w3.org/TR/webrtc/#rtcdatachannelevent
export interface RTCDataChannelEvent extends Event {
  readonly channel: RTCDataChannel;
}

// https://www.w3.org/TR/webrtc/#rtcdtmftonechangeevent
export interface RTCDTMFToneChangeEvent extends Event {
  readonly tone: string;
}

// https://html.spec.whatwg.org/multipage/comms.html#the-messageevent-interface
export interface MessageEvent<T = any> extends Event {
  readonly data: T;
}

// https://www.w3.org/TR/mediacapture-streams/#mediastreamtrackevent
export interface MediaStreamTrackEvent extends Event {
  readonly track: MediaStreamTrack;
}

export interface VideoCapturerStartedEvent extends Event {
  readonly success: boolean;
}

// https://www.w3.org/TR/WebCryptoAPI/#algorithm
export interface Algorithm {
  name: string;
}

// https://www.w3.org/TR/webrtc/#dom-rtcrtptransceiverinit
export interface RTCRtpTransceiverInit {
  direction?: RTCRtpTransceiverDirection;
  streams?: MediaStream[];
  sendEncodings?: RTCRtpEncodingParameters[];
}

// https://www.w3.org/TR/webrtc/#dom-rtcsessiondescriptioninit
export interface RTCSessionDescriptionInit {
  sdp?: string;
  type: RTCSdpType;
}

// https://www.w3.org/TR/webrtc/#rtcsessiondescription-class
export interface RTCSessionDescription {
  readonly sdp: string;
  readonly type: RTCSdpType;

  toJSON(): RTCSessionDescriptionInit;
}

declare var RTCSessionDescription: {
  prototype: RTCSessionDescription;
  new(descriptionInitDict: RTCSessionDescriptionInit): RTCSessionDescription;
};

// https://www.w3.org/TR/webrtc/#dom-rtcicecandidateinit
export interface RTCIceCandidateInit {
  candidate: string;
  sdpMLineIndex?: number;
  sdpMid?: string;
  usernameFragment?: string;
}

// https://www.w3.org/TR/webrtc/#rtcicecandidate-interface
export interface RTCIceCandidate {
  readonly candidate: string;
  readonly sdpMid?: string;
  readonly sdpMLineIndex?: number;
  readonly foundation?: string;
  readonly component?: RTCIceComponent;
  readonly priority?: number;
  readonly address?: string;
  readonly protocol?: RTCIceProtocol;
  readonly port?: number;
  readonly type?: RTCIceCandidateType;
  readonly tcpType?: RTCIceTcpCandidateType;
  readonly relatedAddress?: string;
  readonly relatedPort?: number;
  readonly usernameFragment?: string;

  toJSON(): RTCIceCandidateInit;
}

declare var RTCIceCandidate: {
  prototype: RTCIceCandidate;
  new(candidateInitDict?: RTCIceCandidateInit): RTCIceCandidate;
};

// https://www.w3.org/TR/webrtc/#rtcdatachannel
export interface RTCDataChannel {
  readonly label: string;
  readonly ordered: boolean;
  readonly maxPacketLifeTime?: number;
  readonly maxRetransmits?: number;
  readonly protocol: string;
  readonly negotiated: boolean;
  readonly id?: number;
  readonly readyState: DataChannelState;
  readonly bufferedAmount: number;
  bufferedAmountLowThreshold: number;
  binaryType: BinaryType;

  onbufferedamountlow: ((this: RTCDataChannel, ev: Event) => any) | null;
  onclose: ((this: RTCDataChannel, ev: Event) => any) | null;
  onclosing: ((this: RTCDataChannel, ev: Event) => any) | null;
  onopen: ((this: RTCDataChannel, ev: Event) => any) | null;
  onmessage: ((this: RTCDataChannel, ev: MessageEvent<any>) => any) | null;
  onerror: ((this: RTCDataChannel, ev: RTCErrorEvent) => any) | null;

  close(): void;
  send(data: string): void;
  send(data: ArrayBuffer): void;
}

declare var RTCDataChannel: {
  prototype: RTCDataChannel;
  new(): RTCDataChannel;
};

// https://www.w3.org/TR/webrtc/#dom-rtcdatachannelinit
export interface RTCDataChannelInit {
  ordered?: boolean;
  maxPacketLifeTime?: number;
  maxRetransmits?: number;
  protocol?: string;
  negotiated?: boolean;
  id?: number;
}

// https://www.w3.org/TR/webrtc/#configuration
export interface RTCConfiguration {
  iceServers?: RTCIceServer[];
  iceTransportPolicy?: RTCIceTransportPolicy;
  bundlePolicy?: RTCBundlePolicy;
  rtcpMuxPolicy?: RTCRtcpMuxPolicy;
  certificates?: RTCCertificate[];
  iceCandidatePoolSize?: number;
  // tcpCandidatePolicy
  // continualGatheringPolicy
}

// https://www.w3.org/TR/webrtc/#dom-rtciceserver
export interface RTCIceServer {
  urls: string | string[];
  username?: string;
  credential?: string;
}

// https://www.w3.org/TR/webrtc/#dom-rtcofferansweroptions
export interface RTCOfferAnswerOptions {
}

// https://www.w3.org/TR/webrtc/#dom-rtcofferoptions
export interface RTCOfferOptions extends RTCOfferAnswerOptions {
  iceRestart?: boolean;
}

// https://www.w3.org/TR/webrtc/#dom-rtcansweroptions
export interface RTCAnswerOptions extends RTCOfferAnswerOptions {
}

// https://www.w3.org/TR/webrtc/#rtcpeerconnection-interface
// https://www.w3.org/TR/webrtc/#rtcpeerconnection-interface-extensions
// https://www.w3.org/TR/webrtc/#rtcpeerconnection-interface-extensions-0
// https://www.w3.org/TR/webrtc/#rtcpeerconnection-interface-extensions-1
export interface RTCPeerConnection extends EventTarget {
  readonly canTrickleIceCandidates?: boolean;
  readonly signalingState: RTCSignalingState;
  readonly iceGatheringState: RTCIceGatheringState;
  readonly iceConnectionState: RTCIceConnectionState;
  readonly connectionState: RTCPeerConnectionState;
  readonly localDescription?: RTCSessionDescription;
  readonly remoteDescription?: RTCSessionDescription;
  readonly currentLocalDescription?: RTCSessionDescription;
  readonly currentRemoteDescription?: RTCSessionDescription;
  readonly pendingLocalDescription?: RTCSessionDescription;
  readonly pendingRemoteDescription?: RTCSessionDescription;
  readonly sctp?: RTCSctpTransport;

  onnegotiationneeded: ((this: RTCPeerConnection, ev: Event) => any) | null;
  onicecandidate: ((this: RTCPeerConnection, ev: RTCPeerConnectionIceEvent) => any) | null;
  onicecandidateerror: ((this: RTCPeerConnection, ev: RTCPeerConnectionIceErrorEvent) => any) | null;
  oniceconnectionstatechange: ((this: RTCPeerConnection, ev: Event) => any) | null;
  onicegatheringstatechange: ((this: RTCPeerConnection, ev: Event) => any) | null;
  onsignalingstatechange: ((this: RTCPeerConnection, ev: Event) => any) | null;
  onconnectionstatechange: ((this: RTCPeerConnection, ev: Event) => any) | null;
  ontrack: ((this: RTCPeerConnection, ev: RTCTrackEvent) => any) | null;
  ondatachannel: ((this: RTCPeerConnection, ev: RTCDataChannelEvent) => any) | null;

  addTrack(track: MediaStreamTrack, ...streams: MediaStream[]): RTCRtpSender;
  removeTrack(sender: RTCRtpSender): void;
  setLocalDescription(description?: RTCSessionDescriptionInit): Promise<void>;
  setRemoteDescription(description: RTCSessionDescriptionInit): Promise<void>;
  createOffer(options?: RTCOfferOptions): Promise<RTCSessionDescriptionInit>;
  createAnswer(options?: RTCAnswerOptions): Promise<RTCSessionDescriptionInit>;
  createDataChannel(label: string, dataChannelDict?: RTCDataChannelInit): RTCDataChannel;
  addIceCandidate(candidate?: RTCIceCandidateInit): Promise<void>;
  getSenders(): RTCRtpSender[];
  getReceivers(): RTCRtpReceiver[];
  getTransceivers(): RTCRtpTransceiver[];
  getConfiguration(): RTCConfiguration;
  restartIce(): void;
  setConfiguration(configuration?: RTCConfiguration): void;
  addTransceiver(trackOrKind: MediaStreamTrack | string, init?: RTCRtpTransceiverInit): RTCRtpTransceiver;
  close(): void;
  getStats(selector?: MediaStreamTrack): Promise<RTCStatsReport>;
  setAudioRecording(recording: boolean): void;
  setAudioPlayout(playout: boolean): void;
}

declare var RTCPeerConnection: {
  prototype: RTCPeerConnection;
  new(configuration?: RTCConfiguration): RTCPeerConnection;
  // https://www.w3.org/TR/webrtc/#sec.cert-mgmt
  generateCertificate(keygenAlgorithm: AlgorithmIdentifier): Promise<RTCCertificate>;
};

// https://www.w3.org/TR/webrtc/#rtcrtpreceiver-interface
export interface RTCRtpReceiver {
  readonly track: MediaStreamTrack;
  readonly transport: RTCDtlsTransport | null;

  getParameters(): RTCRtpReceiveParameters;
  getStats(): Promise<RTCStatsReport>;
  getContributingSources(): RTCRtpContributingSource[];
  getSynchronizationSources(): RTCRtpSynchronizationSource[];
}

declare var RTCRtpReceiver: {
  prototype: RTCRtpReceiver;
  new(): RTCRtpReceiver;
  getCapabilities(kind: string): RTCRtpCapabilities | null;
};

// https://www.w3.org/TR/webrtc/#dom-rtcrtpcodingparameters
export interface RTCRtpCodingParameters {
  rid?: string;
}

// https://www.w3.org/TR/webrtc/#dom-rtcrtpencodingparameters
export interface RTCRtpEncodingParameters extends RTCRtpCodingParameters {
  active?: boolean;
  maxBitrate?: number;
  maxFramerate?: number;
  scaleResolutionDownBy?: number;
  // networkPriority?: RTCPriorityType;
  // priority?: RTCPriorityType;
}

// https://www.w3.org/TR/webrtc/#dom-rtcrtpcodecparameters
export interface RTCRtpCodecParameters {
  clockRate: number;
  channels?: number;
  mimeType: string;
  sdpFmtpLine: string;
  payloadType: number;
}

// https://www.w3.org/TR/webrtc/#dom-rtcrtpheaderextensionparameters
export interface RTCRtpHeaderExtensionParameters {
  id: number;
  uri: string;
  encrypted?: boolean;
}

// https://www.w3.org/TR/webrtc/#dom-rtcrtcpparameters
export interface RTCRtcpParameters {
  cname?: string;
  reducedSize?: boolean;
}

// https://www.w3.org/TR/webrtc/#dom-rtcrtpparameters
export interface RTCRtpParameters {
  codecs: RTCRtpCodecParameters[];
  headerExtensions: RTCRtpHeaderExtensionParameters[];
  rtcp: RTCRtcpParameters;
}

// https://www.w3.org/TR/webrtc/#dom-rtcrtpsendparameters
export interface RTCRtpSendParameters extends RTCRtpParameters {
  // degradationPreference?: DegradationPreference;
  encodings: RTCRtpEncodingParameters[];
  transactionId: string;
}

// https://www.w3.org/TR/webrtc/#dom-rtcrtpreceiveparameters
export interface RTCRtpReceiveParameters extends RTCRtpParameters {
}

// https://www.w3.org/TR/webrtc/#dom-rtcrtpcontributingsource
export interface RTCRtpContributingSource {
  timestamp: HighResTimeStamp;
  source: number;
  audioLevel?: number;
  rtpTimestamp: number;
}

// https://www.w3.org/TR/webrtc/#dom-rtcrtpsynchronizationsource
export interface RTCRtpSynchronizationSource extends RTCRtpContributingSource {
}

// https://www.w3.org/TR/webrtc/#rtcrtpsender-interface
// https://www.w3.org/TR/webrtc/#rtcrtpsender-interface-extensions
export interface RTCRtpSender {
  readonly track: MediaStreamTrack | null;
  readonly transport: RTCDtlsTransport | null;
  readonly dtmf: RTCDTMFSender | null;

  setParameters(parameters: RTCRtpSendParameters): Promise<void>;
  getParameters(): RTCRtpSendParameters;
  replaceTrack(withTrack: MediaStreamTrack | null): Promise<void>;
  setStreams(...streams: MediaStream[]): void;
  getStats(): Promise<RTCStatsReport>;
}

declare var RTCRtpSender: {
  prototype: RTCRtpSender;
  new(): RTCRtpSender;
  /**
   * get the most optimistic view of the capabilities of the system for sending media of the given kind.
   * @param kind 'audio' or 'video'.
   * @returns instance of RTCRtpCapabilities, or null if has no capabilities corresponding to the value of the kind argument.
   */
  getCapabilities(kind: string): RTCRtpCapabilities | null;
};

// https://www.w3.org/TR/webrtc/#dom-rtcrtpcodec
export interface RTCRtpCodec {
  mimeType: string;
  clockRate: number;
  channels?: number;
  sdpFmtpLine?: string;
}

// https://www.w3.org/TR/webrtc/#rtcrtpheaderextensioncapability
export interface RTCRtpHeaderExtensionCapability {
  uri: string;
}

// https://www.w3.org/TR/webrtc/#rtcrtpcapabilities
export interface RTCRtpCapabilities {
  codecs: RTCRtpCodec[];
  headerExtensions: RTCRtpHeaderExtensionCapability[];
}

// https://www.w3.org/TR/webrtc/#rtcdtmfsender
export interface RTCDTMFSender extends EventTarget {
  readonly canInsertDTMF: boolean;
  readonly toneBuffer: string;

  ontonechange: ((this: RTCDTMFSender, ev: RTCDTMFToneChangeEvent) => any) | null;

  insertDTMF(tones: string, duration?: number, interToneGap?: number): void;
}

declare var RTCDTMFSender: {
  prototype: RTCDTMFSender;
  new(): RTCDTMFSender;
};

// https://www.w3.org/TR/webrtc/#rtcrtptransceiver-interface
export interface RTCRtpTransceiver {
  readonly mid: string | null;
  readonly sender: RTCRtpSender;
  readonly receiver: RTCRtpReceiver;
  direction: RTCRtpTransceiverDirection;
  readonly currentDirection: RTCRtpTransceiverDirection | null;

  stop(): void;
  setCodecPreferences(codecs: RTCRtpCodec[]): void;
}

declare var RTCRtpTransceiver: {
  prototype: RTCRtpTransceiver;
  new(): RTCRtpTransceiver;
};

// https://www.w3.org/TR/webrtc/#rtcdtlstransport-interface
export interface RTCDtlsTransport extends EventTarget {
  readonly iceTransport: RTCIceTransport;
  readonly state: RTCDtlsTransportState;

  onstatechange: ((this: RTCDtlsTransport, ev: Event) => any) | null;
  onerror: ((this: RTCDtlsTransport, ev: RTCErrorEvent) => any) | null;

  getRemoteCertificates(): ArrayBuffer[];
}

declare var RTCDtlsTransport: {
  prototype: RTCDtlsTransport;
  new(): RTCDtlsTransport;
};

// https://www.w3.org/TR/webrtc/#rtcdtlsfingerprint
export interface RTCDtlsFingerprint {
  algorithm?: string;
  value?: string;
}

// https://www.w3.org/TR/webrtc/#rtccertificate-interface
export interface RTCCertificate {
  readonly expires: EpochTimeStamp;

  getFingerprints(): RTCDtlsFingerprint[];
}

declare var RTCCertificate: {
  prototype: RTCCertificate;
  new(): RTCCertificate;
};

// https://www.w3.org/TR/webrtc/#rtcicetransport
export interface RTCIceTransport extends EventTarget {
  readonly role: RTCIceRole;
  readonly component: RTCIceComponent;
  readonly state: RTCIceTransportState;
  readonly gatheringState: RTCIceGathererState;

  onstatechange: ((this: RTCIceTransport, ev: Event) => any) | null;
  ongatheringstatechange: ((this: RTCIceTransport, ev: Event) => any) | null;
  onselectedcandidatepairchange: ((this: RTCIceTransport, ev: Event) => any) | null;

  getSelectedCandidatePair(): RTCIceCandidatePair | null;
}

declare var RTCIceTransport: {
  prototype: RTCIceTransport;
  new(): RTCIceTransport;
};

// https://www.w3.org/TR/webrtc/#rtcicecandidatepair
export interface RTCIceCandidatePair {
  local?: RTCIceCandidate;
  remote?: RTCIceCandidate;
}

// https://www.w3.org/TR/webrtc/#rtcsctptransport-interface
export interface RTCSctpTransport extends EventTarget {
  readonly maxChannels?: number;
  readonly maxMessageSize: number;
  readonly state: RTCSctpTransportState;
  readonly transport: RTCDtlsTransport;

  onstatechange: ((this: RTCSctpTransport, ev: Event) => any) | null;
}

declare var RTCSctpTransport: {
  prototype: RTCSctpTransport;
  new(): RTCSctpTransport;
};

// https://www.w3.org/TR/webrtc/#rtcstats-dictionary
export interface RTCStats {
  timestamp: HighResTimeStamp;
  type: RTCStatsType;
  id: string;
}

// https://www.w3.org/TR/webrtc/#rtcstatsreport-object
export interface RTCStatsReport {
  readonly stats: Map<string, RTCStats>;
  // readonly timestamp: HighResTimeStamp; // android
  // forEach(callback: (value: RTCStats, key: string, parent: RTCStatsReport) => void): void;
}

// https://www.w3.org/TR/webrtc-stats/#dom-rtctransportstats
export interface RTCTransportStats extends RTCStats {
  packetsSent?: number;
  packetsReceived?: number;
  bytesSent?: number;
  bytesReceived?: number;
  iceRole?: RTCIceRole;
  iceLocalUsernameFragment?: string;
  dtlsState: RTCDtlsTransportState;
  iceState?: RTCIceTransportState;
  selectedCandidatePairId?: string;
  localCertificateId?: string;
  remoteCertificateId?: string;
  tlsVersion?: string;
  dtlsCipher?: string;
  // dtlsRole?: RTCDtlsRole;
  srtpCipher?: string;
  selectedCandidatePairChanges: number;
}

// https://www.w3.org/TR/webrtc-stats/#dom-rtcrtpstreamstats
export interface RTCRtpStreamStats extends RTCStats {
  ssrc: number;
  kind: string;
  transportId?: string;
  codecId?: string;
}

// https://www.w3.org/TR/webrtc-stats/#dom-rtcicecandidatepairstats
export interface RTCIceCandidatePairStats extends RTCStats {
  transportId: string;
  localCandidateId: string;
  remoteCandidateId: string;
  state: RTCStatsIceCandidatePairState;
  nominated?: boolean;
  packetsSent?: number;
  packetsReceived?: number;
  bytesSent?: number;
  bytesReceived?: number;
  lastPacketSentTimestamp?: HighResTimeStamp;
  lastPacketReceivedTimestamp?: HighResTimeStamp;
  totalRoundTripTime?: number;
  currentRoundTripTime?: number;
  availableOutgoingBitrate?: number;
  availableIncomingBitrate?: number;
  requestsReceived?: number;
  requestsSent?: number;
  responsesReceived?: number;
  responsesSent?: number;
  consentRequestsSent?: number;
  packetsDiscardedOnSend?: number;
  bytesDiscardedOnSend?: number;
}

// https://www.w3.org/TR/mediacapture-streams/#media-track-capabilities
export interface MediaTrackCapabilities {
  width?: ULongRange;
  height?: ULongRange;
  aspectRatio?: DoubleRange;
  frameRate?: DoubleRange;
  facingMode?: string[];
  resizeMode?: string[];
  sampleRate?: ULongRange;
  sampleSize?: ULongRange;
  echoCancellation?: boolean[];
  autoGainControl?: boolean[];
  noiseSuppression?: boolean[];
  latency?: DoubleRange;
  channelCount?: ULongRange;
  deviceId?: string;
  groupId?: string;
}

export interface MediaTrackConstraintSet {
  width?: ConstrainULong;
  height?: ConstrainULong;
  aspectRatio?: ConstrainDouble;
  frameRate?: ConstrainDouble;
  facingMode?: ConstrainString;
  resizeMode?: ConstrainString;
  sampleRate?: ConstrainULong;
  sampleSize?: ConstrainULong;
  echoCancellation?: ConstrainBoolean;
  autoGainControl?: ConstrainBoolean;
  noiseSuppression?: ConstrainBoolean;
  latency?: ConstrainDouble;
  channelCount?: ConstrainULong;
  deviceId?: ConstrainString;
  groupId?: ConstrainString;
}


// extensions for screen capture in OpenHarmony
export interface MediaTrackConstraintSet {
  ohosScreenCaptureMode?: ConstrainString;
  ohosScreenCaptureDisplayId?: ConstrainULong;
  ohosScreenCaptureMissionId?: ConstrainString;
  ohosScreenCaptureWindowFilter?: ConstrainString;
  ohosScreenCaptureAudioFilter?: ConstrainString;
  ohosScreenCaptureSkipPrivacyMode?: ConstrainString;
  ohosScreenCaptureAutoRotation?: ConstrainBoolean;
}

// https://www.w3.org/TR/mediacapture-streams/#media-track-constraints
export interface MediaTrackConstraints extends MediaTrackConstraintSet {
  advanced?: MediaTrackConstraintSet[];
}

// https://www.w3.org/TR/mediacapture-streams/#media-track-settings
export interface MediaTrackSettings {
  width?: number;
  height?: number;
  aspectRatio?: number;
  frameRate?: number;
  facingMode?: string;
  resizeMode?: string;
  sampleRate?: number;
  sampleSize?: number;
  echoCancellation?: boolean;
  autoGainControl?: boolean;
  noiseSuppression?: boolean;
  latency?: number;
  channelCount?: number;
  deviceId?: string;
  groupId?: string;
}

// https://www.w3.org/TR/mediacapture-streams/#media-track-supported-constraints
export interface MediaTrackSupportedConstraints {
  width?: boolean;
  height?: boolean;
  aspectRatio?: boolean;
  frameRate?: boolean;
  facingMode?: boolean;
  resizeMode?: boolean;
  sampleRate?: boolean;
  sampleSize?: boolean;
  echoCancellation?: boolean;
  autoGainControl?: boolean;
  noiseSuppression?: boolean;
  latency?: boolean;
  channelCount?: boolean;
  deviceId?: boolean;
  groupId?: boolean;
}

export interface MediaStreamConstraints {
  // default is false
  video?: boolean | MediaTrackConstraints;
  // default is false
  audio?: boolean | MediaTrackConstraints;
}

// https://www.w3.org/TR/screen-capture/#displaymediastreamoptions
export interface DisplayMediaStreamOptions {
  video?: boolean | MediaTrackConstraints;
  audio?: boolean | MediaTrackConstraints;
}

// extension for screen capture in OpenHarmony
export interface DisplayMediaStreamOptions {
  ohosSystemAudio?: boolean | MediaTrackConstraints;
}

export interface MediaSource {
  readonly state: MediaSourceState;
}

export interface AudioSource extends MediaSource {
  setVolume(volume: number);
}

export interface VideoSource extends MediaSource {
  oncapturerstarted: ((this: VideoSource, ev: VideoCapturerStartedEvent) => any) | null;
  oncapturerstopped: ((this: VideoSource, ev: Event) => any) | null;
}

// https://www.w3.org/TR/mediacapture-streams/#mediastreamtrack
export interface MediaStreamTrack extends EventTarget {
  readonly kind: string;
  readonly id: string;
  enabled: boolean;
  readonly readyState: MediaStreamTrackState;

  stop(): void;
  getSource(): MediaSource | null;
}

export declare var MediaStreamTrack: {
  prototype: MediaStreamTrack;
  new(): MediaStreamTrack;
};

export interface AudioTrack extends MediaStreamTrack {
}

export interface VideoTrack extends MediaStreamTrack {
}

// https://www.w3.org/TR/mediacapture-streams/#mediastream
export interface MediaStream extends EventTarget {
  readonly id: string;
  readonly active: boolean;

  addTrack(track: MediaStreamTrack): void;
  removeTrack(track: MediaStreamTrack): void;
  getTrackById(trackId: string): MediaStreamTrack | null;
  getTracks(): MediaStreamTrack[];
  getAudioTracks(): MediaStreamTrack[];
  getVideoTracks(): MediaStreamTrack[];
}

declare var MediaStream: {
  prototype: MediaStream;
  new(): MediaStream;
  new(stream: MediaStream): MediaStream;
  new(tracks: MediaStreamTrack[]): MediaStream;
};

export interface MediaDeviceInfo {
  readonly deviceId: string;
  readonly kind:MediaDeviceKind;
  readonly label: string;
  readonly groupId: string;
}

export interface DeviceChangeEvent extends Event {
  readonly devices: ReadonlyArray<MediaDeviceInfo>;
  readonly userInsertedDevices: ReadonlyArray<MediaDeviceInfo>;
}

// https://www.w3.org/TR/mediacapture-streams/#mediadevices
// https://www.w3.org/TR/screen-capture/#mediadevices-additions
export interface MediaDevices extends EventTarget {
  enumerateDevices(): Promise<MediaDeviceInfo[]>;
  getSupportedConstraints(): MediaTrackSupportedConstraints;
  getUserMedia(constraints?: MediaStreamConstraints): Promise<MediaStream>;
  getDisplayMedia(options?: DisplayMediaStreamOptions): Promise<MediaStream>;
}

declare var MediaDevices: {
  prototype: MediaDevices;
  new(): MediaDevices;
};

export interface NativeVideoRenderer {
  readonly surfaceId?: string;
  readonly videoTrack?: MediaStreamTrack;

  init(surfaceId: string): void;
  setVideoTrack(videoTrack: MediaStreamTrack | null): void;
  setMirror(mirrorHorizontally: boolean): void;
  setMirrorVertically(mirrorVertically: boolean): void;
  setScalingMode(mode: number): void;
  release(): void;
}

declare var NativeVideoRenderer: {
  prototype: NativeVideoRenderer;
  new(): NativeVideoRenderer;
};

export interface AudioError extends Error {
  readonly type: AudioErrorType;
}

export interface AudioErrorEvent extends Event {
  readonly error: AudioError;
}

export interface AudioStateChangeEvent extends Event {
  readonly state: AudioState;
}

export interface AudioCapturerSamplesReadyEvent extends Event {
  readonly samples: AudioSamples;
}

export interface AudioDeviceModuleOptions {
  // input source. see ohos.multimedia.audio.SourceType, default is SOURCE_TYPE_VOICE_COMMUNICATION.
  audioSource?: number;

  // input format. see ohos.multimedia.audio.AudioSampleFormat, default SAMPLE_FORMAT_S16LE.
  audioFormat?: number;

  // input sample rate, default is 48000.
  inputSampleRate?: number;

  // Control if stereo input should be used or not. The default is mono.
  useStereoInput?: boolean;

  // output sample rate, default is 48000.
  outputSampleRate?: number;

  // Control if stereo output should be used or not. The default is mono.
  useStereoOutput?: boolean;

  // output audio usage. see ohos.multimedia.audio.StreamUsage, default is STREAM_USAGE_VOICE_COMMUNICATION
  rendererUsage?: number;

  // enable low latency capturing and rendering, default is false
  useLowLatency?: boolean;

  // Control if the built-in HW acoustic echo canceler should be used or not, default is false.
  // It is possible to query support by calling AudioDeviceModule.isBuiltInAcousticEchoCancelerSupported()
  useHardwareAcousticEchoCanceler?: boolean;

  // Control if the built-in HW noise suppressor should be used or not, default is false.
  // It is possible to query support by calling AudioDeviceModule.isBuiltInNoiseSuppressorSupported()
  useHardwareNoiseSuppressor?: boolean;
}

export interface AudioSamples {
  // See ohos.multimedia.audio.AudioSampleFormat
  readonly audioFormat: number;

  // See ohos.multimedia.audio.AudioChannel
  readonly channelCount: number;

  // See ohos.multimedia.audio.AudioSamplingRate
  readonly sampleRate: number;

  // audio data
  readonly data: ArrayBuffer;
}

export interface AudioDeviceModule {
  oncapturererror: ((this: any, event: AudioErrorEvent) => void) | null;
  oncapturerstatechange: ((this: any, event: AudioStateChangeEvent) => void) | null;
  // Called when new audio samples are ready. This should only be set for debug purposes
  oncapturersamplesready: ((this: any, event: AudioCapturerSamplesReadyEvent) => void) | null;
  onrenderererror: ((this: any, event: AudioErrorEvent) => void) | null;
  onrendererstatechange: ((this: any, event: AudioStateChangeEvent) => void) | null;

  setSpeakerMute(mute: boolean): void;
  setMicrophoneMute(mute: boolean): void;
  setNoiseSuppressorEnabled(enabled: boolean): boolean;
}

declare var AudioDeviceModule: {
  prototype: AudioDeviceModule;
  new(options?: AudioDeviceModuleOptions): AudioDeviceModule;
  isBuiltInAcousticEchoCancelerSupported(): boolean;
  isBuiltInNoiseSuppressorSupported(): boolean;
};

// Hold a native webrtc.AudioProcessing instance
export interface AudioProcessing {}

export interface AudioProcessingFactory {
  create(): AudioProcessing;
}

declare var AudioProcessingFactory: {
  prototype: AudioProcessingFactory;
  new(): AudioProcessingFactory;
};

export interface VideoEncoderFactory {}

export interface VideoDecoderFactory {}

export interface HardwareVideoEncoderFactory extends VideoEncoderFactory {
  readonly enableH264HighProfile: boolean;
}

declare var HardwareVideoEncoderFactory: {
  prototype: HardwareVideoEncoderFactory;
  new(enableH264HighProfile?: boolean): HardwareVideoEncoderFactory;
};

export interface SoftwareVideoEncoderFactory extends VideoEncoderFactory {
}

declare var SoftwareVideoEncoderFactory: {
  prototype: SoftwareVideoEncoderFactory;
  new(): SoftwareVideoEncoderFactory;
};

export interface HardwareVideoDecoderFactory extends VideoDecoderFactory {
}

declare var HardwareVideoDecoderFactory: {
  prototype: HardwareVideoDecoderFactory;
  new(): HardwareVideoDecoderFactory;
};

export interface SoftwareVideoDecoderFactory extends VideoDecoderFactory {
}

declare var SoftwareVideoDecoderFactory: {
  prototype: SoftwareVideoDecoderFactory;
  new(): SoftwareVideoDecoderFactory;
};

export interface PeerConnectionFactoryOptions {
  adm?: AudioDeviceModule;
  videoEncoderFactory?: VideoEncoderFactory;
  videoDecoderFactory?: VideoDecoderFactory;
  audioProcessing?: AudioProcessing;
}

export interface PeerConnectionFactory {
  createPeerConnection(config: RTCConfiguration): RTCPeerConnection;
  createAudioSource(constraints?: MediaTrackConstraints): AudioSource;
  createAudioTrack(id: string, source: AudioSource): AudioTrack;
  createVideoSource(constraints?: MediaTrackConstraints, isScreencast?: boolean): VideoSource;
  createVideoTrack(id: string, source: VideoSource): VideoTrack;
  startAecDump(fd: number, max_size_bytes: number): boolean;
  stopAecDump(): void;
}

declare var PeerConnectionFactory: {
  prototype: PeerConnectionFactory;
  new(options?: PeerConnectionFactoryOptions): PeerConnectionFactory;
  getDefault(): PeerConnectionFactory;
  setDefault(factory: PeerConnectionFactory): void;
};

export interface Loggable {
  logMessage(message: string, severity: number, tag: string): void;
}

export class NativeLogging {
  static injectLoggable(loggable: Loggable, severity: number): void;
  static deleteLoggable(): void;
  static enableLogToDebugOutput(severity): void;
  static enableLogThreads(): void;
  static enableLogTimeStamps(): void;
  static log(message: string, severity: number, tag: string): void;
}
