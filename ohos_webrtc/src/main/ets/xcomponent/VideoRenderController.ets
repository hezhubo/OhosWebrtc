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

import { NativeVideoRenderer, MediaStreamTrack } from 'libohos_webrtc.so';
import { Logging } from '../log/Logging'

const TAG: string = '[VideoRenderController]';

export enum ScalingMode { Fill, AspectFill, AspectFit }

export class VideoRenderController extends XComponentController {
  private renderer: NativeVideoRenderer = new NativeVideoRenderer();

  setVideoTrack(track: MediaStreamTrack | null): void {
    this.renderer.setVideoTrack(track);
  }

  setMirror(mirror: boolean) : void {
    this.renderer.setMirror(mirror);
  }
  
  setMirrorVertically(mirrorVertically: boolean): void {
    this.renderer.setMirrorVertically(mirrorVertically);
  }

  setScalingMode(mode: ScalingMode) : void {
    this.renderer.setScalingMode(mode);
  }

  onSurfaceCreated(surfaceId: string): void {
    Logging.d(TAG, 'onSurfaceCreated surfaceId: ' + surfaceId);
    this.renderer.init(surfaceId);
  }

  onSurfaceChanged(surfaceId: string, rect: SurfaceRect): void
  {
    Logging.d(TAG, 'onSurfaceChanged surfaceId: ' + surfaceId);
    Logging.d(TAG, 'onSurfaceChanged rect: ' + rect);
  }

  onSurfaceDestroyed(surfaceId: string): void
  {
    Logging.d(TAG, 'onSurfaceDestroyed surfaceId: ' + surfaceId);
    this.renderer.release();
  }
}
