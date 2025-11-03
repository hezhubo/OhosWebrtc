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

import {Loggable, NativeLogging} from 'libohos_webrtc.so'

export enum LoggingSeverity { VERBOSE, INFO, WARNING, ERROR, NONE }

export class Logging {
  private static loggingEnabled: boolean = false;
  private static loggable: Loggable | null;
  private static loggableSeverity: LoggingSeverity = LoggingSeverity.ERROR;

  static injectLoggable(injectedLoggable: Loggable, severity: LoggingSeverity) {
    if (injectedLoggable != null) {
      Logging.loggable = injectedLoggable;
      Logging.loggableSeverity = severity;
      NativeLogging.injectLoggable(injectedLoggable, severity);
    }
  }

  static deleteInjectedLoggable() {
    Logging.loggable = null;
    NativeLogging.deleteLoggable();
  }

  static enableLogThreads() {
    NativeLogging.enableLogThreads();
  }

  static enableLogTimeStamps() {
    NativeLogging.enableLogTimeStamps();
  }

  static enableLogToDebugOutput(severity: LoggingSeverity) {
    if (Logging.loggable != null) {
      throw new Error("Logging to native debug output not supported while Loggable is injected."
        + "Delete the Loggable before calling this method.");
    }
    NativeLogging.enableLogToDebugOutput(severity);
    Logging.loggingEnabled = true;
  }

  static log(severity: LoggingSeverity, tag: string, message: string) {
    if (Logging.loggable) {
      // Filter log messages below loggableSeverity.
      if (severity < Logging.loggableSeverity) {
        return;
      }

      Logging.loggable.logMessage(message, severity, tag);
      return;
    }

    // Try native logging if no loggable is injected.
    if (Logging.loggingEnabled) {
      NativeLogging.log(message, severity, tag);
      return;
    }

    // Fallback to system 
    switch (severity) {
      case LoggingSeverity.ERROR:
        console.error(tag, message);
        break;
      case LoggingSeverity.WARNING:
        console.warn(tag, message);
        break;
      case LoggingSeverity.INFO:
        console.info(tag, message);
        break;
      default:
        console.debug(tag, message);
        break;
    }
  }

  static d(tag: string, message: string) {
    Logging.log(LoggingSeverity.INFO, tag, message);
  }

  static e(tag: string, message: string, e?: Error) {
    Logging.log(LoggingSeverity.ERROR, tag, message);
    if (e != undefined) {
      Logging.log(LoggingSeverity.ERROR, tag, e.message);
      // Logging.log(LoggingSeverity.ERROR, tag, e.stack);
    }
  }

  static w(tag: string, message: string, e: Error) {
    Logging.log(LoggingSeverity.WARNING, tag, message);
    if (e != undefined) {
      Logging.log(LoggingSeverity.WARNING, tag, e.message);
      // Logging.log(LoggingSeverity.WARNING, tag, e.stack);
    }
  }

  static v(tag: string, message: string) {
    Logging.log(LoggingSeverity.VERBOSE, tag, message);
  }

}
