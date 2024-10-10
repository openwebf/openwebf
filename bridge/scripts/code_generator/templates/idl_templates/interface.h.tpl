#include "<%= blob.implement %>.h"

<% if(parentClassName) { %>
#include "qjs_<%= _.snakeCase(parentClassName) %>.h"
<% } %>

namespace webf {

class ExecutingContext;

<% if (className != "Event" && className != "CustomEvent" && _.endsWith(className, "Event")){ %>

// Dart generated nativeEvent member are force align to 64-bit system. So all members in NativeEvent should have 64 bit
// width.
#if ANDROID_32_BIT
struct Native<%= className %> {
  Native<%= parentClassName %> native_event;
  <% _.forEach(object.props, function(prop, index) { %>
  <% if (prop.typeMode.static) { return; } %>
<%= generateRawTypeValue(prop.type, true) %> <%= prop.name %>;
  <% }) %>
};
#else
// Use pointer instead of int64_t on 64 bit system can help compiler to choose best register for better running
// performance.
struct Native<%= className %> {
Native<%= parentClassName %> native_event;
<% _.forEach(object.props, function(prop, index) { %>
<% if (prop.typeMode.static) { return; } %>
<%= generateRawTypeValue(prop.type) %> <%= prop.name %>;
<% }) %>
};
#endif
<% } %>

class QJS<%= className %> : public QJSInterfaceBridge<QJS<%= className %>, <%= className%>> {
 public:
  static void Install(ExecutingContext* context);
  static WrapperTypeInfo* GetWrapperTypeInfo() {
    return const_cast<WrapperTypeInfo*>(&wrapper_type_info_);
  }
  <% if (object.construct) { %> static JSValue ConstructorCallback(JSContext* ctx, JSValue func_obj, JSValue this_val, int argc, JSValue* argv, int flags); <% } %>
  static const WrapperTypeInfo wrapper_type_info_;
 private:
 <% if (globalFunctionInstallList.length > 0) { %> static void InstallGlobalFunctions(ExecutingContext* context); <% } %>
 <% if (classMethodsInstallList.length > 0) { %> static void InstallPrototypeMethods(ExecutingContext* context); <% } %>
 <% if (classPropsInstallList.length > 0) { %> static void InstallPrototypeProperties(ExecutingContext* context); <% } %>
 <% if (object.construct) { %> static void InstallConstructor(ExecutingContext* context); <% } %>

 <% if (object.indexedProp) { %>
  static int PropertyEnumerateCallback(JSContext* ctx, JSPropertyEnum** ptab, uint32_t* plen, JSValueConst obj);
  static bool PropertyCheckerCallback(JSContext* ctx, JSValueConst obj, JSAtom atom);
  <% if (object.indexedProp.indexKeyType == 'number') { %>
  static JSValue IndexedPropertyGetterCallback(JSContext* ctx, JSValue obj, uint32_t index);
  <% } else { %>
  static JSValue StringPropertyGetterCallback(JSContext* ctx, JSValue obj, JSAtom key);
  <% } %>
  <% if (!object.indexedProp.readonly) { %>
    <% if (object.indexedProp.indexKeyType == 'number') { %>
  static bool IndexedPropertySetterCallback(JSContext* ctx, JSValueConst obj, uint32_t index, JSValueConst value);
    <% } else { %>
  static bool StringPropertySetterCallback(JSContext* ctx, JSValueConst obj, JSAtom key, JSValueConst value);
    <% } %>
    static bool StringPropertyDeleterCallback(JSContext* ctx, JSValueConst obj, JSAtom key);
  <% } %>
 <% } %>
};


}
