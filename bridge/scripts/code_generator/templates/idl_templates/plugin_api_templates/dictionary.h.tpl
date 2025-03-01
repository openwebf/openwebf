#include "webf_value.h"
#include "foundation/native_value.h"

namespace webf {

<% _.forEach(dependentTypes, function (dependentType) { %>
  <% if (dependentType.endsWith('Options') || dependentType.endsWith('Init')) { %>
  <% } else if (dependentType === 'JSEventListener') { %>
  <% } else { %>
class <%= dependentType %>;
typedef struct <%= dependentType %>PublicMethods <%= dependentType %>PublicMethods;
  <% } %>
<% }); %>

struct WebF<%= className %> {

<% _.forEach(parentObjects, function(parentObject, index) { %>
  <% if (parentObject?.props) { %>
    <% _.forEach(parentObject.props, function(prop, index) { %>
      <% if (isStringType(prop.type)) { %>
  <%= generatePublicReturnTypeValue(prop.type, true) %> <%= _.snakeCase(prop.name) %>;
      <% } else if (prop.readonly) { %>
  const <%= generatePublicReturnTypeValue(prop.type, true) %> <%= _.snakeCase(prop.name) %>;
      <% } else { %>
  <%= generatePublicReturnTypeValue(prop.type, true) %> <%= _.snakeCase(prop.name) %>;
      <% } %>
    <% }); %>
  <% } %>
<% }); %>

<% _.forEach(object.props, function(prop, index) { %>
  <% if (isStringType(prop.type)) { %>
  <%= generatePublicReturnTypeValue(prop.type, true) %> <%= _.snakeCase(prop.name) %>;
  <% } else if (prop.readonly) { %>
  const <%= generatePublicReturnTypeValue(prop.type, true) %> <%= _.snakeCase(prop.name) %>;
  <% } else { %>
  <%= generatePublicReturnTypeValue(prop.type, true) %> <%= _.snakeCase(prop.name) %>;
  <% } %>
<% }); %>
};

}  // namespace webf
