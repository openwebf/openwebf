import { EventInit } from "../dom/events/event_init";

// @ts-ignore
@Dictionary()
export interface DeviceorientationEventInit extends EventInit {
    absolute?: boolean;
    alpha?: number;
    beta?: number;
    gamma?: number;
}
