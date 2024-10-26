#[repr(C)]
pub struct <%= className %>RustMethods {
  pub version: c_double,
  <% if (object.parent) { %>
  pub <%= _.snakeCase(object.parent) %>: *const <%= object.parent %>RustMethods,
  <% } %>

  <% _.forEach(object.props, function(prop, index) { %>
    <% var propName = generateValidRustIdentifier(_.snakeCase(prop.name)); %>
  pub <%= propName %>: extern "C" fn(ptr: *const OpaquePtr) -> <%= generatePublicReturnTypeValue(prop.type) %>,
    <% if (!prop.readonly) { %>
  pub set_<%= _.snakeCase(prop.name) %>: extern "C" fn(ptr: *const OpaquePtr, value: <%= generatePublicReturnTypeValue(prop.type) %>, exception_state: *const OpaquePtr) -> bool,
    <% } %>
  <% }); %>

  <% _.forEach(object.methods, function(method, index) { %>
    <% var methodName = generateValidRustIdentifier(_.snakeCase(method.name)); %>
  pub <%= methodName %>: extern "C" fn(ptr: *const OpaquePtr, <%= generatePublicParametersType(method.args) %>exception_state: *const OpaquePtr) -> <%= generatePublicReturnTypeValue(method.returnType) %>,
  <% }); %>

  <% if (!object.parent) { %>
  pub release: extern "C" fn(ptr: *const OpaquePtr) -> c_void,
  <% } %>
}

<% if (object.parent) { %>
pub struct <%= className %> {
  pub <%= _.snakeCase(object.parent) %>: <%= object.parent %>,
  method_pointer: *const <%= className %>RustMethods,
}
<% } else { %>
pub struct <%= className %> {
  pub ptr: *const OpaquePtr,
  context: *const ExecutingContext,
  method_pointer: *const <%= className %>RustMethods,
  status: *const RustValueStatus
}
<% } %>

impl <%= className %> {
  <% if (object.parent) { %>
  pub fn initialize(ptr: *const OpaquePtr, context: *const ExecutingContext, method_pointer: *const <%= className %>RustMethods, status: *const RustValueStatus) -> <%= className %> {
    unsafe {
      <%= className %> {
        <%= _.snakeCase(object.parent) %>: <%= object.parent %>::initialize(
          ptr,
          context,
          method_pointer.as_ref().unwrap().<%= _.snakeCase(object.parent) %>,
          status,
        ),
        method_pointer,
      }
    }
  }

  pub fn ptr(&self) -> *const OpaquePtr {
    self.<%= _.snakeCase(object.parent) %>.ptr()
  }

  pub fn context<'a>(&self) -> &'a ExecutingContext {
    self.<%= _.snakeCase(object.parent) %>.context()
  }

  <% } else { %>
  pub fn initialize(ptr: *const OpaquePtr, context: *const ExecutingContext, method_pointer: *const <%= className %>RustMethods, status: *const RustValueStatus) -> <%= className %> {
    <%= className %> {
      ptr,
      context,
      method_pointer,
      status
    }
  }

  pub fn ptr(&self) -> *const OpaquePtr {
    self.ptr
  }

  pub fn context<'a>(&self) -> &'a ExecutingContext {
    assert!(!self.context.is_null(), "Context PTR must not be null");
    unsafe { &*self.context }
  }

  <% } %>

  <% _.forEach(object.props, function(prop, index) { %>
    <% var propName = generateValidRustIdentifier(_.snakeCase(prop.name)); %>
    <% if (isVoidType(prop.type)) { %>
  pub fn <%= propName %>(&self) {
    unsafe {
      ((*self.method_pointer).<%= propName %>)(self(.ptr()));
    };
  }
    <% } else { %>
  pub fn <%= propName %>(&self) -> <%= generateMethodReturnType(prop.type) %> {
    let value = unsafe {
      ((*self.method_pointer).<%= propName %>)(self.ptr())
    };
    <%= generatePropReturnStatements(prop.type) %>
  }
    <% } %>

    <% if (!prop.readonly) { %>
  pub fn set_<%= _.snakeCase(prop.name) %>(&self, value: <%= generateMethodReturnType(prop.type) %>, exception_state: &ExceptionState) -> Result<(), String> {
    unsafe {
      ((*self.method_pointer).set_<%= _.snakeCase(prop.name) %>)(self.ptr(), <%= generateMethodParametersName([{name: 'value', type: prop.type}]) %>exception_state.ptr)
    };
    if exception_state.has_exception() {
      return Err(exception_state.stringify(self.context()));
    }
    Ok(())
  }
    <% } %>
  <% }); %>

  <% _.forEach(object.methods, function(method, index) { %>
    <% var methodName = generateValidRustIdentifier(_.snakeCase(method.name)); %>
    <% if (isVoidType(method.returnType)) { %>
  pub fn <%= methodName %>(&self, <%= generateMethodParametersTypeWithName(method.args) %>exception_state: &ExceptionState) -> Result<(), String> {
    unsafe {
      ((*self.method_pointer).<%= methodName %>)(self.ptr(), <%= generateMethodParametersName(method.args) %>exception_state.ptr);
    };
    if exception_state.has_exception() {
      return Err(exception_state.stringify(self.context()));
    }
    Ok(())
  }
    <% } else { %>
  pub fn <%= methodName %>(&self, <%= generateMethodParametersTypeWithName(method.args) %>exception_state: &ExceptionState) -> Result<<%= generateMethodReturnType(method.returnType) %>, String> {
    let value = unsafe {
      ((*self.method_pointer).<%= methodName %>)(self.ptr(), <%= generateMethodParametersName(method.args) %>exception_state.ptr)
    };
    if exception_state.has_exception() {
      return Err(exception_state.stringify(self.context()));
    }
    <%= generateMethodReturnStatements(method.returnType) %>
  }
    <% } %>
  <% }); %>
}

<% if (!object.parent) { %>
impl Drop for <%= className %> {
  fn drop(&mut self) {
    unsafe {
      ((*self.method_pointer).release)(self.ptr());
    }
  }
}
<% } %>

<% var parentMethodsSuperTrait = object.parent ? `: ${object.parent}Methods` : ''; %>
pub trait <%= className %>Methods<%= parentMethodsSuperTrait %> {
  <% _.forEach(object.props, function(prop, index) { %>
    <% var propName = generateValidRustIdentifier(_.snakeCase(prop.name)); %>
    <% if (isVoidType(prop.type)) { %>
  fn <%= propName %>(&self);
    <% } else { %>
  fn <%= propName %>(&self) -> <%= generateMethodReturnType(prop.type) %>;
    <% } %>

    <% if (!prop.readonly) { %>
  fn set_<%= _.snakeCase(prop.name) %>(&self, value: <%= generateMethodReturnType(prop.type) %>, exception_state: &ExceptionState) -> Result<(), String>;
    <% } %>
  <% }); %>

  <% _.forEach(object.methods, function(method, index) { %>
    <% var methodName = generateValidRustIdentifier(_.snakeCase(method.name)); %>
    <% if (isVoidType(method.returnType)) { %>
  fn <%= methodName %>(&self, <%= generateMethodParametersTypeWithName(method.args) %>exception_state: &ExceptionState) -> Result<(), String>;
    <% } else { %>
  fn <%= methodName %>(&self, <%= generateMethodParametersTypeWithName(method.args) %>exception_state: &ExceptionState) -> Result<<%= generateMethodReturnType(method.returnType) %>, String>;
    <% } %>
  <% }); %>
  fn as_<%= _.snakeCase(className) %>(&self) -> &<%= className %>;
}

impl <%= className %>Methods for <%= className %> {
  <% _.forEach(object.props, function(prop, index) { %>
    <% var propName = generateValidRustIdentifier(_.snakeCase(prop.name)); %>
    <% if (isVoidType(prop.type)) { %>
  fn <%= propName %>(&self) {
    self.<%= propName %>()
  }
    <% } else { %>
  fn <%= propName %>(&self) -> <%= generateMethodReturnType(prop.type) %> {
    self.<%= propName %>()
  }
    <% } %>

    <% if (!prop.readonly) { %>
  fn set_<%= _.snakeCase(prop.name) %>(&self, value: <%= generateMethodReturnType(prop.type) %>, exception_state: &ExceptionState) -> Result<(), String> {
    self.set_<%= _.snakeCase(prop.name) %>(value, exception_state)
  }
    <% } %>
  <% }); %>

  <% _.forEach(object.methods, function(method, index) { %>
    <% var methodName = generateValidRustIdentifier(_.snakeCase(method.name)); %>
    <% if (isVoidType(method.returnType)) { %>
  fn <%= methodName %>(&self, <%= generateMethodParametersTypeWithName(method.args) %>exception_state: &ExceptionState) -> Result<(), String> {
    self.<%= methodName %>(<%= generateParentMethodParametersName(method.args) %>exception_state)
  }
    <% } else { %>
  fn <%= methodName %>(&self, <%= generateMethodParametersTypeWithName(method.args) %>exception_state: &ExceptionState) -> Result<<%= generateMethodReturnType(method.returnType) %>, String> {
    self.<%= methodName %>(<%= generateParentMethodParametersName(method.args) %>exception_state)
  }
    <% } %>
  <% }); %>
  fn as_<%= _.snakeCase(className) %>(&self) -> &<%= className %> {
    self
  }
}

<% var parentKey = ''; %>
<% _.forEach(inheritedObjects, function (parentObject) { %>
  <% parentKey = parentKey === '' ? _.snakeCase(parentObject.name) : `${parentKey}.${_.snakeCase(parentObject.name)}`; %>
impl <%= parentObject.name %>Methods for <%= className %> {
  <% _.forEach(parentObject.props, function(prop, index) { %>
    <% var propName = generateValidRustIdentifier(_.snakeCase(prop.name)); %>
    <% if (isVoidType(prop.type)) { %>
  fn <%= propName %>(&self) {
    self.<%= parentKey %>.<%= propName %>()
  }
    <% } else { %>
  fn <%= propName %>(&self) -> <%= generateMethodReturnType(prop.type) %> {
    self.<%= parentKey %>.<%= propName %>()
  }
    <% } %>

    <% if (!prop.readonly) { %>
  fn set_<%= _.snakeCase(prop.name) %>(&self, value: <%= generateMethodReturnType(prop.type) %>, exception_state: &ExceptionState) -> Result<(), String> {
    self.<%= parentKey %>.set_<%= _.snakeCase(prop.name) %>(value, exception_state)
  }
    <% } %>
  <% }); %>

  <% _.forEach(parentObject.methods, function(method, index) { %>
    <% var methodName = generateValidRustIdentifier(_.snakeCase(method.name)); %>
    <% if (isVoidType(method.returnType)) { %>
  fn <%= methodName %>(&self, <%= generateMethodParametersTypeWithName(method.args) %>exception_state: &ExceptionState) -> Result<(), String> {
    self.<%= parentKey %>.<%= methodName %>(<%= generateParentMethodParametersName(method.args) %>exception_state)
  }
    <% } else { %>
  fn <%= methodName %>(&self, <%= generateMethodParametersTypeWithName(method.args) %>exception_state: &ExceptionState) -> Result<<%= generateMethodReturnType(method.returnType) %>, String> {
    self.<%= parentKey %>.<%= methodName %>(<%= generateParentMethodParametersName(method.args) %>exception_state)
  }
    <% } %>
  <% }); %>
  fn as_<%= _.snakeCase(parentObject.name) %>(&self) -> &<%= parentObject.name %> {
    &self.<%= parentKey %>
  }
}
<% }); %>
