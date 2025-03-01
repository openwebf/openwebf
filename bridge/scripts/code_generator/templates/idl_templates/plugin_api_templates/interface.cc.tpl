<% if (className.endsWith('Event')) { %>
#include "plugin_api/<%= _.snakeCase(className) %>_init.h"
<% }%>
namespace webf {

<% _.forEach(object.props, function(prop, index) { %>
<%= generatePublicReturnTypeValue(prop.type, true) %> <%= className %>PublicMethods::<%= _.startCase(prop.name).replace(/ /g, '') %>(<%= className %>* <%= _.snakeCase(className) %><%= isAnyType(prop.type)? ", SharedExceptionState* shared_exception_state": "" %>) {
  <% if (isPointerType(prop.type)) { %>
  auto* result = <%= _.snakeCase(className) %>-><%= prop.name %>();
  WebFValueStatus* status_block = result->KeepAlive();
  return <%= generatePublicReturnTypeValue(prop.type, true) %>(result, result-><%= _.camelCase(getPointerType(prop.type)) %>PublicMethods(), status_block);
  <% } else if (isAnyType(prop.type)) { %>
  auto value = <%= _.snakeCase(className) %>-><%= prop.name %>();
  auto native_value = value.ToNative(<%= _.snakeCase(className) %>->ctx(), shared_exception_state->exception_state, false);
  return native_value;
  <% } else if (isStringType(prop.type)) { %>
  return <%= _.snakeCase(className) %>-><%= prop.name %>().ToStringView().Characters8();
  <% } else { %>
  return <%= _.snakeCase(className) %>-><%= prop.name %>();
  <% } %>
}
  <% if (!prop.readonly) { %>
void <%= className %>PublicMethods::Set<%= _.startCase(prop.name).replace(/ /g, '') %>(<%= className %>* <%= _.snakeCase(className) %>, <%= generatePublicReturnTypeValue(prop.type, true) %> <%= prop.name %>, SharedExceptionState* shared_exception_state) {
  <% if (isStringType(prop.type)) { %>
  webf::AtomicString <%= prop.name %>Atomic = webf::AtomicString(<%= _.snakeCase(className) %>->ctx(), <%= prop.name %>);
  <% } %>
  <%= _.snakeCase(className) %>->set<%= _.startCase(prop.name).replace(/ /g, '') %>(<%= prop.name %><% if (isStringType(prop.type)) { %>Atomic<% } %>, shared_exception_state->exception_state);
}
  <% } %>
  <% if (isStringType(prop.type)) { %>
<%= generatePublicReturnTypeValue(prop.type, true) %> <%= className %>PublicMethods::Dup<%= _.startCase(prop.name).replace(/ /g, '') %>(<%= className %>* <%= _.snakeCase(className) %>) {
  const char* buffer = <%= _.snakeCase(className) %>-><%= prop.name %>().ToStringView().Characters8();
  return strdup(buffer);
}
  <% } %>
<% }); %>

<% _.forEach(object.methods, function(method, index) { %>
<%= generatePublicReturnTypeValue(method.returnType, true) %> <%= className %>PublicMethods::<%= _.startCase(method.name).replace(/ /g, '') %>(<%= className %>* <%= _.snakeCase(className) %>, <%= generatePublicParametersTypeWithName(method.args, true) %>SharedExceptionState* shared_exception_state) {
  <% _.forEach(method.args, function(arg, index) { %>
    <% if (isStringType(arg.type)) { %>
  webf::AtomicString <%= _.snakeCase(arg.name) %>_atomic = webf::AtomicString(<%= _.snakeCase(className) %>->ctx(), <%= _.snakeCase(arg.name) %>);
    <% } %>
    <% if (isAnyType(arg.type)) { %>
  ScriptValue <%=_.snakeCase(arg.name)%>_script_value = ScriptValue(<%= _.snakeCase(className) %>->ctx(), <%=_.snakeCase(arg.name)%>);
    <% } %>
  <% }); %>
    <% if (isStringType(method.returnType)) { %>
  webf::AtomicString value = <%= _.snakeCase(className) %>-><%= method.name %>(<%= generatePublicParametersName(method.args) %>shared_exception_state->exception_state);
  return value.ToStringView().Characters8();
    <% } else if (isVoidType(method.returnType)) { %>
  <%= _.snakeCase(className) %>-><%= method.name %>(<%= generatePublicParametersName(method.args) %>shared_exception_state->exception_state);
    <% } else if (isAnyType(method.returnType)) { %>
  auto return_value = <%= _.snakeCase(className) %>-><%= method.name %>(<%= generatePublicParametersName(method.args) %>shared_exception_state->exception_state);
  auto return_native_value = return_value.ToNative(<%= _.snakeCase(className) %>->ctx(), shared_exception_state->exception_state, false);
  return return_native_value;
    <% } else { %>
  return <%= _.snakeCase(className) %>-><%= method.name %>(<%= generatePublicParametersName(method.args) %>shared_exception_state->exception_state);
    <% } %>
}
<% }); %>

<% if (!object.parent) { %>
void <%= className %>PublicMethods::Release(<%= className %>* <%= _.snakeCase(className) %>) {
  <%= _.snakeCase(className) %>->ReleaseAlive();
}

WebFValue<<%= className %>, WebFPublicMethods> <%= className %>PublicMethods::DynamicTo(webf::<%= className %>* <%= _.snakeCase(className) %>, webf::<%= className %>Type <%= _.snakeCase(className) %>_type) {
  switch (<%= _.snakeCase(className) %>_type) {
    case <%= className %>Type::k<%= className %>: {
      WebFValueStatus* status_block = <%= _.snakeCase(className) %>->KeepAlive();
      return WebFValue<<%= className %>, WebFPublicMethods>(<%= _.snakeCase(className) %>, <%= _.snakeCase(className) %>-><%= _.camelCase(className) %>PublicMethods(), status_block);
    }
  <% _.forEach(subClasses, function (subClass, index) { %>
    case <%= className %>Type::k<%= subClass %>: {
      auto* <%= _.snakeCase(subClass) %> = webf::DynamicTo<<%= subClass %>>(<%= _.snakeCase(className) %>);
      if (<%= _.snakeCase(subClass) %> == nullptr) {
        return WebFValue<<%= className %>, WebFPublicMethods>::Null();
      }
      WebFValueStatus* status_block = <%= _.snakeCase(subClass) %>->KeepAlive();
      return WebFValue<<%= className %>, WebFPublicMethods>(<%= _.snakeCase(subClass) %>, <%= _.snakeCase(subClass) %>-><%= _.camelCase(subClass) %>PublicMethods(), status_block);
    }
  <% }); %>
    default:
      assert_m(false, ("Unknown <%= className %>Type " + std::to_string(static_cast<int32_t>(<%= _.snakeCase(className) %>_type))).c_str());
      return WebFValue<<%= className %>, WebFPublicMethods>::Null();
  }
}
<% } %>
<% if (object.construct && !isVoidType(object.construct.returnType)) { %>
  <% if (object.construct.args.length === 0) { %>
WebFValue<<%= className %>, <%= className %>PublicMethods> ExecutingContextWebFMethods::Create<%= className %>(ExecutingContext* context, ExceptionState& exception_state) {

  <%= className %>* obj = <%= className %>::Create(context, exception_state);
  WebFValueStatus* status_block = obj->KeepAlive();

  return WebFValue<<%= className %>, <%= className %>PublicMethods>(obj, obj-><%= _.camelCase(className) %>PublicMethods(), status_block);
};
  <% } %>

  <% if (object.construct.args.length >= 1 && object.construct.args.some(arg => arg.name === 'type')) { %>
WebFValue<<%= className %>, <%= className %>PublicMethods> ExecutingContextWebFMethods::Create<%= className %>(ExecutingContext* context, const char* type, ExceptionState& exception_state) {
  AtomicString type_atomic = AtomicString(context->ctx(), type);

  <%= className %>* event = <%= className %>::Create(context, type_atomic, exception_state);

  WebFValueStatus* status_block = event->KeepAlive();
  return WebFValue<<%= className %>, <%= className %>PublicMethods>(event, event-><%= _.camelCase(className) %>PublicMethods(), status_block);
};
  <% } %>

  <% if (object.construct.args.length > 1) { %>
WebFValue<<%= className %>, <%= className %>PublicMethods> ExecutingContextWebFMethods::Create<%= className %>WithOptions(ExecutingContext* context, <%= generatePublicParametersTypeWithName(object.construct.args, true) %> ExceptionState& exception_state) {
  <% if (object.construct.args.some(arg => arg.name === 'type')) { %>
  AtomicString type_atomic = AtomicString(context->ctx(), type);
  <% } %>
  std::shared_ptr<<%= className %>Init> init_class = <%= className %>Init::Create();
  <% if(dependentClasses[className + 'Init']){ %>
  <% _.forEach([...dependentClasses[className + 'Init'].props, ...dependentClasses[className + 'Init'].inheritedProps], function (prop) { %>
  <% if(isStringType(prop.type)) { %>
  AtomicString <%=_.snakeCase(prop.name)%>_atomic = AtomicString(context->ctx(), init-><%=_.snakeCase(prop.name)%>);
  init_class->set<%=_.upperFirst(prop.name)%>(<%=_.snakeCase(prop.name)%>_atomic);
  <% } else if (isPointerType(prop.type)) { %>
  init_class->set<%=_.upperFirst(prop.name)%>(init-><%=_.snakeCase(prop.name)%>.value);
  <% } else if (isAnyType(prop.type)) { %>
  NativeValue <%=_.snakeCase(prop.name)%> = init-><%=_.snakeCase(prop.name)%>;
  ScriptValue <%=_.snakeCase(prop.name)%>_script_value = ScriptValue(context->ctx(), <%=_.snakeCase(prop.name)%>);
  init_class->set<%=_.upperFirst(prop.name)%>(<%=_.snakeCase(prop.name)%>_script_value);
  <% } else { %>
  init_class->set<%=_.upperFirst(prop.name)%>(init-><%=_.snakeCase(prop.name)%>);
  <% } %>
  <% }) %>
  <% } %>

  <%= className %>* event = <%= className %>::Create(context, <% if (object.construct.args.some(arg => arg.name === 'type')) { %>type_atomic<% } %>, init_class, exception_state);

  WebFValueStatus* status_block = event->KeepAlive();
  return WebFValue<<%= className %>, <%= className %>PublicMethods>(event, event-><%= _.camelCase(className) %>PublicMethods(), status_block);
};
<% } %>
<% } %>
}  // namespace webf
