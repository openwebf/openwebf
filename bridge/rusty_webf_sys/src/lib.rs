/*
* Copyright (C) 2022-present The WebF authors. All rights reserved.
*/
#![allow(unused)]

pub mod css;
pub mod dom;
pub mod events;
pub mod frame;
pub mod geometry;
pub mod html;
pub mod input;
pub mod timing;

pub mod exception_state;
pub mod executing_context;
mod memory_utils;
pub mod native_value;
pub mod script_value_ref;
pub mod webf_event_listener;
pub mod webf_function;
pub mod webf_future;

pub use css::*;
pub use dom::*;
pub use events::*;
pub use frame::*;
pub use geometry::*;
pub use html::*;
pub use input::*;
pub use timing::*;

pub use exception_state::*;
pub use executing_context::*;
pub use native_value::*;
pub use script_value_ref::*;
pub use webf_event_listener::*;
pub use webf_function::*;
pub use webf_future::*;

#[repr(C)]
pub struct OpaquePtr;

#[repr(C)]
pub struct RustValueStatus {
  pub disposed: bool,
}

#[repr(C)]
pub struct RustValue<T> {
  pub value: *const OpaquePtr,
  pub method_pointer: *const T,
  pub status: *const RustValueStatus,
}

pub fn initialize_webf_api(value: RustValue<ExecutingContextRustMethods>, meta_data: *const NativeLibraryMetaData) -> ExecutingContext {
  ExecutingContext::initialize(value.value, value.method_pointer, meta_data, value.status)
}

// This is the entrypoint when your rust app compiled as dynamic library and loaded & executed by WebF.
// #[no_mangle]
// pub extern "C" fn load_webf_rust_module(context: *mut c_void, method_pointer: *const c_void) {
//
// }
