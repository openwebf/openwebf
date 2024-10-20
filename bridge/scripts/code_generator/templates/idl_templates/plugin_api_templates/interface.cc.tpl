namespace webf {

<% _.forEach(object.props, function(prop, index) { %>
<%= generatePublicReturnTypeValue(prop.type, true) %> <%= className %>PublicMethods::<%= _.startCase(prop.name).replace(/ /g, '') %>(<%= className %>* <%= _.snakeCase(className) %>) {
  <% if (isPointerType(prop.type)) { %>
  auto* result = <%= _.snakeCase(className) %>-><%= prop.name %>();
  result->KeepAlive();
  return {.value = result, .method_pointer = result-><%= _.camelCase(getPointerType(prop.type)) %>PublicMethods()};
  <% } else if (isAnyType(prop.type)) { %>
  return {
        .value = new ScriptValueRef {
          <%= _.snakeCase(className) %>->GetExecutingContext(),
          <%= _.snakeCase(className) %>-><%= prop.name %>()
        },
        .method_pointer = ScriptValueRef::publicMethods(),
    };
  <% } else if (isStringType(prop.type)) { %>
  return <%= _.snakeCase(className) %>-><%= prop.name %>().ToStringView().Characters8();
  <% } else { %>
  return <%= _.snakeCase(className) %>-><%= prop.name %>();
  <% } %>
}
  <% if (!prop.readonly) { %>
void <%= className %>PublicMethods::Set<%= _.startCase(prop.name).replace(/ /g, '') %>(<%= className %>* <%= _.snakeCase(className) %>, <%= generatePublicReturnTypeValue(prop.type, true) %> <%= prop.name %>, SharedExceptionState* shared_exception_state) {
  <%= _.snakeCase(className) %>->set<%= _.startCase(prop.name).replace(/ /g, '') %>(<%= prop.name %>, shared_exception_state->exception_state);
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
  <% }); %>
  return <%= _.snakeCase(className) %>-><%= method.name %>(<%= generatePublicParametersName(method.args) %>shared_exception_state->exception_state);
}
<% }); %>

<% if (!object.parent) { %>
void <%= className %>PublicMethods::Release(<%= className %>* <%= _.snakeCase(className) %>) {
  <%= _.snakeCase(className) %>->ReleaseAlive();
}
<% } %>

}  // namespace webf
