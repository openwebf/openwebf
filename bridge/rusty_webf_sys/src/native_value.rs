use std::ffi::*;
use std::mem;
#[cfg(target_os = "windows")]
use windows::Win32::System::Com::{CoTaskMemAlloc, CoTaskMemFree};

use crate::memory_utils::safe_free_cpp_ptr;

#[repr(C)]
pub struct SharedNativeString {
  pub string_: *mut u16,
  pub length_: u32,
}

#[repr(C)]
pub enum NativeTag {
  TagString = 0,
  TagInt = 1,
  TagBool = 2,
  TagNull = 3,
  TagFloat64 = 4,
  TagJson = 5,
  TagList = 6,
  TagPointer = 7,
  TagFunction = 8,
  TagAsyncFunction = 9,
  TagUint8Bytes = 10,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub union ValueField {
  pub int64: i64,
  pub float64: f64,
  pub ptr: *mut c_void,
}

#[repr(C)]
#[derive(Clone)]
pub struct NativeValue {
  pub u: ValueField,
  pub uint32: u32,
  pub tag: i32,
}

impl NativeValue {
  pub fn new() -> Self {
    let size = mem::size_of::<NativeValue>();

    #[cfg(target_os = "windows")]
    let ptr = unsafe { CoTaskMemAlloc(size) };

    #[cfg(not(target_os = "windows"))]
    let ptr = unsafe { libc::malloc(size) };

    let ptr = ptr as *mut NativeValue;
    let value = unsafe { ptr.read() };
    value
  }

  fn create_string_ptr(val: &str, len: usize) -> *mut SharedNativeString {
    let size = len * mem::size_of::<u16>();

    #[cfg(target_os = "windows")]
    let ptr = unsafe { CoTaskMemAlloc(size) };

    #[cfg(not(target_os = "windows"))]
    let ptr = unsafe { libc::malloc(size) };

    let ptr = ptr as *mut u16;

    for (i, c) in val.encode_utf16().enumerate() {
      unsafe {
        ptr.add(i).write(c);
      }
    }

    let mut shared_string = SharedNativeString {
      string_: ptr,
      length_: len as u32,
    };

    let shared_string_size = mem::size_of::<SharedNativeString>();

    #[cfg(target_os = "windows")]
    let shared_string_ptr = unsafe { CoTaskMemAlloc(shared_string_size) };

    #[cfg(not(target_os = "windows"))]
    let shared_string_ptr = unsafe { libc::malloc(shared_string_size) };

    let shared_string_ptr = shared_string_ptr as *mut SharedNativeString;
    unsafe {
      shared_string_ptr.write(shared_string);
    }

    shared_string_ptr
  }

  pub fn new_string(val: &str) -> Self {
    let len = val.len();
    let shared_string_ptr = Self::create_string_ptr(val, len);
    let mut value = Self::new();
    value.tag = NativeTag::TagString as i32;
    value.u.ptr = shared_string_ptr as *mut c_void;
    value.uint32 = 0;
    value
  }

  pub fn is_string(&self) -> bool {
    self.tag == NativeTag::TagString as i32
  }

  pub fn to_string(&self) -> String {
    let ptr = unsafe {
      self.u.ptr as *mut SharedNativeString
    };
    let string_struct = unsafe { ptr.read() };
    let slice = unsafe { std::slice::from_raw_parts(string_struct.string_, string_struct.length_.try_into().unwrap()) };
    String::from_utf16_lossy(slice)
  }

  pub fn new_null() -> Self {
    let mut value = Self::new();
    value.tag = NativeTag::TagNull as i32;
    value.u.int64 = 0;
    value.uint32 = 0;
    value
  }

  pub fn is_null(&self) -> bool {
    self.tag == NativeTag::TagNull as i32
  }

  pub fn new_float64(val: f64) -> Self {
    let mut value = Self::new();
    value.tag = NativeTag::TagFloat64 as i32;
    value.u.float64 = val;
    value.uint32 = 0;
    value
  }

  pub fn is_float64(&self) -> bool {
    self.tag == NativeTag::TagFloat64 as i32
  }

  pub fn to_float64(&self) -> f64 {
    unsafe {
      self.u.float64
    }
  }

  pub fn new_bool(val: bool) -> Self {
    let mut value = Self::new();
    value.tag = NativeTag::TagBool as i32;
    value.u.int64 = if val { 1 } else { 0 };
    value.uint32 = 0;
    value
  }

  pub fn is_bool(&self) -> bool {
    self.tag == NativeTag::TagBool as i32
  }

  pub fn to_bool(&self) -> bool {
    unsafe {
      self.u.int64 != 0
    }
  }

  pub fn new_int64(val: i64) -> Self {
    let mut value = Self::new();
    value.tag = NativeTag::TagInt as i32;
    value.u.int64 = val;
    value.uint32 = 0;
    value
  }

  pub fn is_int64(&self) -> bool {
    self.tag == NativeTag::TagInt as i32
  }

  pub fn to_int64(&self) -> i64 {
    unsafe {
      self.u.int64
    }
  }

  pub fn new_list(values: Vec<NativeValue>) -> Self {
    let size = values.len();
    let array_size = size * mem::size_of::<NativeValue>();

    #[cfg(target_os = "windows")]
    let array_ptr = unsafe { CoTaskMemAlloc(array_size) };

    #[cfg(not(target_os = "windows"))]
    let array_ptr = unsafe { libc::malloc(array_size) };

    let array_ptr = array_ptr as *mut NativeValue;

    for (i, val) in values.iter().enumerate() {
      let mut value = val.clone();
      unsafe {
        array_ptr.add(i).write(value);
      }
    }

    let mut value = Self::new();
    value.tag = NativeTag::TagList as i32;
    value.u.ptr = array_ptr as *mut c_void;
    value.uint32 = size as u32;
    value
  }

  pub fn is_list(&self) -> bool {
    self.tag == NativeTag::TagList as i32
  }

  pub fn to_list(&self) -> Vec<NativeValue> {
    let mut values = Vec::new();
    let ptr = unsafe {
      self.u.ptr as *mut NativeValue
    };
    for i in 0..self.uint32 {
      let offset = i.try_into().unwrap();
      let val = unsafe { ptr.add(offset).read() };
      values.push(val);
    }
    values
  }

  pub fn new_u8_bytes(values: Vec<u8>) -> Self {
    let size = values.len();
    let array_size = size * mem::size_of::<u8>();

    #[cfg(target_os = "windows")]
    let array_ptr = unsafe { CoTaskMemAlloc(array_size) };

    #[cfg(not(target_os = "windows"))]
    let array_ptr = unsafe { libc::malloc(array_size) };

    let array_ptr = array_ptr as *mut u8;

    for (i, val) in values.iter().enumerate() {
      unsafe {
        array_ptr.add(i).write(*val);
      }
    }

    let mut value = Self::new();
    value.tag = NativeTag::TagUint8Bytes as i32;
    value.u.ptr = array_ptr as *mut c_void;
    value.uint32 = size as u32;
    value
  }

  pub fn is_u8_bytes(&self) -> bool {
    self.tag == NativeTag::TagUint8Bytes as i32
  }

  pub fn to_u8_bytes(&self) -> Vec<u8> {
    let mut values = Vec::new();
    let ptr = unsafe {
      self.u.ptr as *mut u8
    };
    for i in 0..self.uint32 {
      let offset = i.try_into().unwrap();
      let val = unsafe { ptr.add(offset).read() };
      values.push(val);
    }
    values
  }

  pub fn new_json(val: &str) -> Self {
    let len = val.len();
    let shared_string_ptr = Self::create_string_ptr(val, len);
    let mut value = Self::new();
    value.tag = NativeTag::TagJson as i32;
    value.u.ptr = shared_string_ptr as *mut c_void;
    value.uint32 = 0;
    value
  }

  pub fn is_json(&self) -> bool {
    self.tag == NativeTag::TagJson as i32
  }

  pub fn to_json(&self) -> String {
    let ptr = unsafe {
      self.u.ptr as *mut SharedNativeString
    };
    let string_struct = unsafe { ptr.read() };
    let slice = unsafe { std::slice::from_raw_parts(string_struct.string_, string_struct.length_.try_into().unwrap()) };
    String::from_utf16_lossy(slice)
  }
}

impl Drop for NativeValue {
  fn drop(&mut self) {
    // no need to drop inner structure, it will be freed by the dart side
  }
}
