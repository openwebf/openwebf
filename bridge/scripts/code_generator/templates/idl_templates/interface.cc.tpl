<% if (object.construct) { %>
JSValue QJS<%= className %>::ConstructorCallback(JSContext* ctx, JSValue func_obj, JSValue this_val, int argc, JSValue* argv, int flags) {
  <%= generateFunctionBody(blob, object.construct, {isConstructor: true}) %>
}
<% } %>

<% if (object.indexedProp) { %>
  bool QJS<%= className %>::PropertyCheckerCallback(JSContext* ctx, JSValueConst obj, JSAtom key) {
    auto* self = toScriptWrappable<<%= className %>>(obj);
    ExceptionState exception_state;
    ExecutingContext* context = ExecutingContext::From(ctx);
    if (!context->IsContextValid()) return false;
    context->dartIsolateContext()->profiler()->StartTrackSteps("QJS<%= className %>::PropertyCheckerCallback");
    auto* wrapper_type_info = DOMTokenList::GetStaticWrapperTypeInfo();
    MemberMutationScope scope{context};
    JSValue prototype = context->contextData()->prototypeForType(wrapper_type_info);
    if (JS_HasProperty(ctx, prototype, key)) return true;
    bool result = self->NamedPropertyQuery(AtomicString(ctx, key), exception_state);
    context->dartIsolateContext()->profiler()->FinishTrackSteps();
    if (UNLIKELY(exception_state.HasException())) {
      return false;
    }
    return result;
  }
  int QJS<%= className %>::PropertyEnumerateCallback(JSContext* ctx, JSPropertyEnum** ptab, uint32_t* plen, JSValue obj) {
    auto* self = toScriptWrappable<<%= className %>>(obj);
    ExceptionState exception_state;
    ExecutingContext* context = ExecutingContext::From(ctx);
    if (!context->IsContextValid()) return 0;
    MemberMutationScope scope{context};
    context->dartIsolateContext()->profiler()->StartTrackSteps("QJS<%= className %>::PropertyEnumerateCallback");
    std::vector<AtomicString> props;
    self->NamedPropertyEnumerator(props, exception_state);
    auto size = props.size() == 0 ? 1 : props.size();
    auto tabs = (JSPropertyEnum *)js_malloc(ctx, sizeof(JSPropertyEnum *) * size);
    for(int i = 0; i < props.size(); i ++) {
      tabs[i].atom = JS_DupAtom(ctx, props[i].Impl());
      tabs[i].is_enumerable = true;
    }

    *plen = props.size();
    *ptab = tabs;
    context->dartIsolateContext()->profiler()->FinishTrackSteps();
    return 0;
  }

  <% if (object.indexedProp.indexKeyType == 'number') { %>
  JSValue QJS<%= className %>::IndexedPropertyGetterCallback(JSContext* ctx, JSValue obj, uint32_t index) {
    ExceptionState exception_state;
    ExecutingContext* context = ExecutingContext::From(ctx);
    if (!context->IsContextValid()) return JS_NULL;
    MemberMutationScope scope{context};
    auto* self = toScriptWrappable<<%= className %>>(obj);
    if (index >= self->length()) {
      return JS_UNDEFINED;
    }
    context->dartIsolateContext()->profiler()->StartTrackSteps("QJS<%= className %>::IndexedPropertyGetterCallback");
    <%= generateCoreTypeValue(object.indexedProp.type) %> result = self->item(index, exception_state);
    context->dartIsolateContext()->profiler()->FinishTrackSteps();
    if (UNLIKELY(exception_state.HasException())) {
      return exception_state.ToQuickJS();
    }

    return Converter<<%= generateIDLTypeConverter(object.indexedProp.type, object.indexedProp.optional) %>>::ToValue(ctx, result);
  };
  <% } else { %>
  JSValue QJS<%= className %>::StringPropertyGetterCallback(JSContext* ctx, JSValue obj, JSAtom key) {
    auto* self = toScriptWrappable<<%= className %>>(obj);
    ExceptionState exception_state;
    ExecutingContext* context = ExecutingContext::From(ctx);
    if (!context->IsContextValid()) return JS_NULL;
    context->dartIsolateContext()->profiler()->StartTrackSteps("QJS<%= className %>::StringPropertyGetterCallback");
    MemberMutationScope scope{context};
    ${generateCoreTypeValue(object.indexedProp.type)} result = self->item(AtomicString(ctx, key), exception_state);
    context->dartIsolateContext()->profiler()->FinishTrackSteps();
    if (UNLIKELY(exception_state.HasException())) {
      return exception_state.ToQuickJS();
    }
    return Converter<<%= generateIDLTypeConverter(object.indexedProp.type, object.indexedProp.optional) %>>::ToValue(ctx, result);
  };
  <% } %>
  <% if (!object.indexedProp.readonly) { %>
    <% if (object.indexedProp.indexKeyType == 'number') { %>
  bool QJS<%= className %>::IndexedPropertySetterCallback(JSContext* ctx, JSValueConst obj, uint32_t index, JSValueConst value) {
    auto* self = toScriptWrappable<<%= className %>>(obj);
    ExceptionState exception_state;
    ExecutingContext* context = ExecutingContext::From(ctx);
    if (!context->IsContextValid()) return false;
    MemberMutationScope scope{context};
    auto&& v = Converter<<%= generateIDLTypeConverter(object.indexedProp.type, object.indexedProp.optional) %>>::FromValue(ctx, value, exception_state);
    if (UNLIKELY(exception_state.HasException())) {
      return false;
    }
    context->dartIsolateContext()->profiler()->StartTrackSteps("QJS<%= className %>::IndexedPropertySetterCallback");
    bool success = self->SetItem(index, v, exception_state);
    context->dartIsolateContext()->profiler()->FinishTrackSteps();
    if (UNLIKELY(exception_state.HasException())) {
      return false;
    }
    return success;
  };
    <% } else { %>
  bool QJS<%= className %>::StringPropertySetterCallback(JSContext* ctx, JSValueConst obj, JSAtom key, JSValueConst value) {
    auto* self = toScriptWrappable<<%= className %>>(obj);
    ExceptionState exception_state;
    ExecutingContext* context = ExecutingContext::From(ctx);
    if (!context->IsContextValid()) return false;
    MemberMutationScope scope{context};
    auto&& v = Converter<<%= generateIDLTypeConverter(object.indexedProp.type, object.indexedProp.optional) %>>::FromValue(ctx, value, exception_state);
    if (UNLIKELY(exception_state.HasException())) {
      return false;
    }
    context->dartIsolateContext()->profiler()->StartTrackSteps("QJS<%= className %>::StringPropertySetterCallback");
    bool success = self->SetItem(AtomicString(ctx, key), v, exception_state);
    context->dartIsolateContext()->profiler()->FinishTrackSteps();
    if (UNLIKELY(exception_state.HasException())) {
      return false;
    }
    return success;
  };
    <% } %>
     bool QJS<%= className %>::StringPropertyDeleterCallback(JSContext* ctx, JSValueConst obj, JSAtom key) {
      auto* self = toScriptWrappable<<%= className %>>(obj);
      ExceptionState exception_state;
      ExecutingContext* context = ExecutingContext::From(ctx);
      if (!context->IsContextValid()) return false;
      MemberMutationScope scope{context};
      if (UNLIKELY(exception_state.HasException())) {
        return false;
      }
      context->dartIsolateContext()->profiler()->StartTrackSteps("QJS<%= className %>::StringPropertyDeleterCallback");
      bool success = self->DeleteItem(AtomicString(ctx, key), exception_state);
      context->dartIsolateContext()->profiler()->FinishTrackSteps();
      if (UNLIKELY(exception_state.HasException())) {
        return false;
      }
      return success;
    };
  <% } %>
 <% } %>

<% _.forEach(filtedMethods, function(method, index) { %>

  <% if (overloadMethods[method.name] && overloadMethods[method.name].length > 1) { %>
    <% _.forEach(overloadMethods[method.name], function(overloadMethod, index) { %>
static JSValue <%= overloadMethod.name %>_overload_<%= index %>(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
        <%= generateFunctionBody(blob, overloadMethod, {isInstanceMethod: true}) %>
      }
    <% }); %>
    static JSValue qjs_<%= method.name %>(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
      <%= generateOverLoadSwitchBody(overloadMethods[method.name]) %>
    }
  <% } else { %>

  static JSValue qjs_<%= method.name %>(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
    <%= generateFunctionBody(blob, method, {isInstanceMethod: true}) %>
  }
  <% } %>

<% }) %>

<% _.forEach(object.props, function(prop, index) { %>
static JSValue <%= prop.name %>AttributeGetCallback(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
<% if (isJSArrayBuiltInProps(prop)) { %>
  JSValue classProto = JS_GetClassProto(ctx, JS_CLASS_ARRAY);
  <% if (prop.isSymbol) { %>
  JSValue result = JS_GetProperty(ctx, classProto, JS_ATOM_<%= prop.name %>);
  JS_FreeValue(ctx, classProto);
  return result;
  <% } else { %>
  JSValue result = JS_GetPropertyStr(ctx, classProto, "<%= prop.name %>");
  JS_FreeValue(ctx, classProto);
  return result;
  <% } %>

<% } else { %>

  auto* <%= blob.filename %> = toScriptWrappable<<%= className %>>(this_val);
  assert(<%= blob.filename %> != nullptr);
  ExecutingContext* context = ExecutingContext::From(ctx);
  if (!context->IsContextValid()) return JS_NULL;
  MemberMutationScope scope{context};
  context->dartIsolateContext()->profiler()->StartTrackSteps("<%= className %>::<%= prop.name %>");

  <% if (prop.typeMode && prop.typeMode.dartImpl) { %>
  ExceptionState exception_state;
  <% if (isTypeNeedAllocate(prop.type)) { %>
  typename <%= generateNativeValueTypeConverter(prop.type) %>::ImplType v = NativeValueConverter<<%= generateNativeValueTypeConverter(prop.type) %>>::FromNativeValue(ctx, <%= blob.filename %>->GetBindingProperty(binding_call_methods::k<%= prop.name %>, FlushUICommandReason::kDependentsOnElement  <%= prop.typeMode.layoutDependent ? '| FlushUICommandReason::kDependentsOnLayout' : '' %>, exception_state));
  <% } else { %>
  typename <%= generateNativeValueTypeConverter(prop.type) %>::ImplType v = NativeValueConverter<<%= generateNativeValueTypeConverter(prop.type) %>>::FromNativeValue(<%= blob.filename %>->GetBindingProperty(binding_call_methods::k<%= prop.name %>, FlushUICommandReason::kDependentsOnElement  <%= prop.typeMode.layoutDependent ? '| FlushUICommandReason::kDependentsOnLayout' : '' %>, exception_state));
  <% } %>
  if (UNLIKELY(exception_state.HasException())) {
    context->dartIsolateContext()->profiler()->FinishTrackSteps();
    return exception_state.ToQuickJS();
  }
  auto result = Converter<<%= generateIDLTypeConverter(prop.type, prop.optional) %>>::ToValue(ctx, v);
  context->dartIsolateContext()->profiler()->FinishTrackSteps();
  return result;
  <% } else if (prop.typeMode && prop.typeMode.static) { %>
  auto result = Converter<<%= generateIDLTypeConverter(prop.type, prop.optional) %>>::ToValue(ctx, <%= className %>::<%= prop.name %>);
  context->dartIsolateContext()->profiler()->FinishTrackSteps();
  return result;
  <% } else { %>
  auto result = Converter<<%= generateIDLTypeConverter(prop.type, prop.optional) %>>::ToValue(ctx, <%= blob.filename %>-><%= prop.name %>());
  context->dartIsolateContext()->profiler()->FinishTrackSteps();
  return result;
  <% } %>

<% } %>
}
<% if (!prop.readonly) { %>
static JSValue <%= prop.name %>AttributeSetCallback(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
 auto* <%= blob.filename %> = toScriptWrappable<<%= className %>>(this_val);
  ExceptionState exception_state;
  ExecutingContext* context = ExecutingContext::From(ctx);
  if (!context->IsContextValid()) return JS_NULL;
  MemberMutationScope scope{context};
  auto&& v = Converter<<%= generateIDLTypeConverter(prop.type, prop.optional) %>>::FromValue(ctx, argv[0], exception_state);
  if (exception_state.HasException()) {
    return exception_state.ToQuickJS();
  }
  <% if (prop.typeMode && prop.typeMode.dartImpl) { %>
  <%= blob.filename %>->SetBindingProperty(binding_call_methods::k<%= prop.name %>, NativeValueConverter<<%= generateNativeValueTypeConverter(prop.type) %>>::ToNativeValue(<% if (isDOMStringType(prop.type)) { %>ctx, <% } %>v),exception_state);
  <% } else {%>
  <%= blob.filename %>->set<%= prop.name[0].toUpperCase() + prop.name.slice(1) %>(v, exception_state);
  <% } %>
  if (exception_state.HasException()) {
    return exception_state.ToQuickJS();
  }

  return JS_DupValue(ctx, argv[0]);
}
<% } %>
<% }); %>


<% if (mixinObjects) { %>
<% mixinObjects.forEach(function(object) { %>

<% _.forEach(object.props, function(prop, index) { %>
static JSValue <%= prop.name %>AttributeGetCallback(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  auto* <%= blob.filename %> = toScriptWrappable<<%= className %>>(this_val);
  assert(<%= blob.filename %> != nullptr);
  ExecutingContext* context = ExecutingContext::From(ctx);
  if (!context->IsContextValid()) return JS_NULL;
  MemberMutationScope scope{context};
  return Converter<<%= generateIDLTypeConverter(prop.type, prop.optional) %>>::ToValue(ctx, <%= object.name %>::<%= prop.name %>(*<%= blob.filename %>));
}
<% if (!prop.readonly) { %>
static JSValue <%= prop.name %>AttributeSetCallback(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
 auto* <%= blob.filename %> = toScriptWrappable<<%= className %>>(this_val);
  ExceptionState exception_state;
  ExecutingContext* context = ExecutingContext::From(ctx);
  if (!context->IsContextValid()) return JS_NULL;
  MemberMutationScope scope{context};
  auto&& v = Converter<<%= generateIDLTypeConverter(prop.type, prop.optional) %>>::FromValue(ctx, argv[0], exception_state);
  if (exception_state.HasException()) {
    return exception_state.ToQuickJS();
  }

  <%= object.name %>::set<%= prop.name[0].toUpperCase() + prop.name.slice(1) %>(*<%= blob.filename %>, v, exception_state);
  if (exception_state.HasException()) {
    return exception_state.ToQuickJS();
  }

  return JS_DupValue(ctx, argv[0]);
}
<% } %>
<% }); %>


<% }); %>
<% } %>
