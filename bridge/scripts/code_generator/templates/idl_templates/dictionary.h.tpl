
<% if (object.parent) { %>
#include "qjs_<%= _.snakeCase(object.parent) %>.h"
<% } %>

namespace webf {

class ExecutingContext;
class ExceptionState;
class Node;

class <%= className %> : public <%= object.parent ? object.parent : 'DictionaryBase' %> {
 public:
  using ImplType = std::shared_ptr<<%= className %>>;
  static std::shared_ptr<<%= className %>> Create();
  static std::shared_ptr<<%= className %>> Create(JSContext* ctx, JSValue value, ExceptionState& exception_state);
  explicit <%= className %>();
  explicit <%= className %>(JSContext* ctx, JSValue value, ExceptionState& exception_state);

  <% _.forEach(props, (function(prop, index) { %>
  <%= generateCoreTypeValue(prop.type) %> <%= prop.name %>() const {
    assert(has_<%= prop.name %>_);
    return <%= prop.name %>_;
  }
  bool has<%= prop.name[0].toUpperCase() + prop.name.slice(1) %>() const { return has_<%= prop.name %>_; }
  void set<%= prop.name[0].toUpperCase() + prop.name.slice(1) %>(<%= generateCoreTypeValue(prop.type) %> value) {
    <%= prop.name %>_ = value;
    has_<%= prop.name %>_ = true;
  }
  <% })); %>
  bool FillQJSObjectWithMembers(JSContext *ctx, JSValue qjs_dictionary) const override;
  bool FillMembersWithQJSObject(JSContext* ctx, JSValue value, ExceptionState& exception_state) override;
private:
  <% _.forEach(props, (function(prop, index) { %>
  <%= generateCoreTypeValue(prop.type) %> <%= prop.name %>_;
  bool has_<%= prop.name %>_ = false;
  <% })); %>
};

}
