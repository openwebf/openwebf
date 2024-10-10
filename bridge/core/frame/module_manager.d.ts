import {ModuleManagerOptions} from "./module_manager_options";

declare const __webf_invoke_module__: (moduleName: string, methodName: string, paramsValue?: any, callback?: Function) => any;
declare const __webf_invoke_module_with_options__: (moduleName: string, methodName: string, paramsValue: any, options: ModuleManagerOptions, callback: Function) => any;
declare const __webf_add_module_listener__: (moduleName: string, callback: Function) => void;
declare const __webf_remove_module_listener__: (moduleName: string) => void;
declare const __webf_clear_module_listener__: () => void;
