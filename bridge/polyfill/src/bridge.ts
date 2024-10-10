/*
* Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/

declare const __webf_invoke_module__: (module: string, method: string, params?: any | null, fn?: (err: Error, data: any) => any) => any;
export const webfInvokeModule = __webf_invoke_module__;

declare const __webf_invoke_module_with_options__: (module: string, method: string, params?: any | null, options?: any, fn?: (err: Error, data: any) => any) => any
export const webfInvokeModuleWithOptions = __webf_invoke_module_with_options__;

declare const __webf_add_module_listener__: (moduleName: string, fn: (event: Event, extra: any) => any) => void;
export const addWebfModuleListener = __webf_add_module_listener__;

declare const __webf_clear_module_listener__: () => void;
export const clearWebfModuleListener = __webf_clear_module_listener__;

declare const __webf_remove_module_listener__: (name: string) => void;
export const removeWebfModuleListener = __webf_remove_module_listener__;

declare const __webf_location_reload__: () => void;
export const webfLocationReload = __webf_location_reload__;

declare const __webf_print__: (log: string, level?: string) => void;
export const webfPrint = __webf_print__;

declare const __webf_is_proxy__: (obj: any) => boolean;

export const webfIsProxy = __webf_is_proxy__;