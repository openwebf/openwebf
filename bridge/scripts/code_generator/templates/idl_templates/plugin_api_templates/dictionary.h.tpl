#include "webf_value.h"

namespace webf {

<% _.forEach(dependentTypes, function (dependentType) { %>
  <% if (dependentType.endsWith('Options') || dependentType.endsWith('Init')) { %>
  <% } else if (dependentType === 'JSEventListener') { %>
  <% } else { %>
typedef struct <%= dependentType %> <%= dependentType %>;
typedef struct <%= dependentType %>PublicMethods <%= dependentType %>PublicMethods;
  <% } %>
<% }); %>

struct WebF<%= className %> {
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
