import {IDLBlob} from "./IDLBlob";
import {
  ClassObject,
  FunctionArguments,
  FunctionArgumentType,
  FunctionDeclaration,
  FunctionObject,
  ParameterMode,
  PropsDeclaration,
} from "./declaration";
import {addIndent, getClassName, getWrapperTypeInfoNameOfClassName} from "./utils";
import {ParameterType} from "./analyzer";
import _ from 'lodash';
import fs from 'fs';
import path from 'path';
import {getTemplateKind, TemplateKind} from "./generateHeader";
import {GenerateOptions} from "./generator";
import {
  generateTypeRawChecker,
  generateUnionConstructorImpl,
  generateUnionMemberName,
  generateUnionTypeClassName,
  generateUnionTypeClear,
  generateUnionTypeFileName,
  generateUnionTypeSetter,
  getUnionTypeName
} from "./generateUnionTypes";

const dictionaryClasses: string[] = [];

function generateMethodArgumentsCheck(m: FunctionDeclaration) {
  if (m.args.length == 0) return '';

  let requiredArgsCount = 0;
  m.args.forEach(m => {
    if (m.required) requiredArgsCount++;
  });

  if (requiredArgsCount > 0 && m.args[0].isDotDotDot) {
    return '';
  }

  return `  if (argc < ${requiredArgsCount}) {
    return JS_ThrowTypeError(ctx, "Failed to execute '${m.name}' : ${requiredArgsCount} argument required, but %d present.", argc);
  }
`;
}

export function isTypeNeedAllocate(type: ParameterType) {
  switch (type.value) {
    case FunctionArgumentType.undefined:
    case FunctionArgumentType.null:
    case FunctionArgumentType.int32:
    case FunctionArgumentType.int64:
    case FunctionArgumentType.boolean:
    case FunctionArgumentType.double:
      return false;
    default:
      return true;
  }
}

export function generateCoreTypeValue(type: ParameterType): string {
  switch (type.value) {
    case FunctionArgumentType.int64: {
      return 'int64_t';
    }
    case FunctionArgumentType.int32: {
      return 'int32_t';
    }
    case FunctionArgumentType.void: {
      return 'void';
    }
    case FunctionArgumentType.double: {
      return 'double';
    }
    case FunctionArgumentType.boolean: {
      return 'bool';
    }
    case FunctionArgumentType.dom_string:
    case FunctionArgumentType.legacy_dom_string: {
      return 'AtomicString';
    }
    case FunctionArgumentType.any: {
      return 'ScriptValue';
    }
  }

  if (isDictionary(type)) {
    return `std::shared_ptr<${getPointerType(type)}>`;
  }

  if (isPointerType(type)) {
    return getPointerType(type) + '*';
  }

  if (type.isArray && typeof type.value === 'object' && !Array.isArray(type.value)) {
    return `std::vector<${generateCoreTypeValue(type.value)}>`;
  }

  return '';
}

export function generateRawTypeValue(type: ParameterType, is32Bit: boolean = false): string {
  switch (type.value) {
    case FunctionArgumentType.int64: {
      return 'int64_t';
    }
    case FunctionArgumentType.int32: {
      return 'int64_t';
    }
    case FunctionArgumentType.double: {
      return 'double';
    }
    case FunctionArgumentType.boolean: {
      return 'int64_t';
    }
    case FunctionArgumentType.dom_string:
    case FunctionArgumentType.legacy_dom_string: {
      if (is32Bit) {
        return 'int64_t';
      }

      return 'SharedNativeString*';
    }
    default:
      if (is32Bit) {
        return 'int64_t';
      }
      return 'void*';
  }

  if (isPointerType(type)) {
    if (is32Bit) {
      return 'int64_t';
    }
    return 'NativeBindingObject*';
  }

  return '';
}

export function isTypeHaveNull(type: ParameterType): boolean {
  if (type.isArray) return false;
  if (!Array.isArray(type.value)) {
    return type.value === FunctionArgumentType.null;
  }
  return type.value.some(t => t.value === FunctionArgumentType.null);
}

export function isTypeHaveString(types: ParameterType[]): boolean {
  return types.some(t => {
    if (t.isArray) return isTypeHaveString(t.value as ParameterType[]);
    if (!Array.isArray(t.value)) {
      return t.value === FunctionArgumentType.dom_string;
    }
    return t.value.some(t => t.value === FunctionArgumentType.dom_string);
  });
}

export function isPointerType(type: ParameterType): boolean {
  if (type.isArray) return false;
  if (typeof type.value === 'string') {
    return true;
  }
  if (Array.isArray(type.value)) {
    return type.value.some(t => typeof t.value === 'string');
  }
  return false;
}

export function isDictionary(type: ParameterType): boolean {
  if (type.isArray) return false;
  if (typeof type.value === 'string') {
    return dictionaryClasses.indexOf(type.value) >= 0;
  }
  return false;
}

export function getPointerType(type: ParameterType): string {
  if (typeof type.value === 'string') {
    return type.value;
  }
  if (Array.isArray(type.value)) {
    for (let i = 0; i < type.value.length; i++) {
      let childValue = type.value[i];
      if (typeof childValue.value === 'string') {
        return childValue.value;
      }
    }
  }
  return '';
}

export function isUnionType(type: ParameterType): boolean {
  if (type.isArray || !Array.isArray(type.value)) {
    return false;
  }

  const trimedType = trimNullTypeFromType(type);
  return Array.isArray(trimedType.value);
}

export function trimNullTypeFromType(type: ParameterType): ParameterType {
  let types = type.value;
  if (!Array.isArray(types)) return type;
  let trimed = types.filter(t => t.value != FunctionArgumentType.null);

  if (trimed.length === 1) {
    return {
      isArray: false,
      value: trimed[0].value
    }
  }

  return {
    isArray: type.isArray,
    value: trimed
  };
}

export function generateIDLTypeConverter(type: ParameterType, isOptional?: boolean): string {
  let haveNull = isTypeHaveNull(type);
  let returnValue = '';

  if (type.isArray) {
    returnValue = `IDLSequence<${generateIDLTypeConverter(type.value as ParameterType, isOptional)}>`;
  } else if (isUnionType(type) && Array.isArray(type.value)) {
    returnValue = generateUnionTypeClassName(type.value);
  } else if (isPointerType(type)) {
    returnValue = getPointerType(type);
  } else {
    type = trimNullTypeFromType(type);
    switch (type.value) {
      case FunctionArgumentType.int32:
        returnValue = `IDLInt32`;
        break;
      case FunctionArgumentType.int64:
        returnValue = 'IDLInt64';
        break;
      case FunctionArgumentType.double:
        returnValue = `IDLDouble`;
        break;
      case FunctionArgumentType.function:
        returnValue = `IDLCallback`;
        break;
      case FunctionArgumentType.boolean:
        returnValue = `IDLBoolean`;
        break;
      case FunctionArgumentType.dom_string:
        returnValue = `IDLDOMString`;
        break;
      case FunctionArgumentType.object:
        returnValue = `IDLObject`;
        break;
      case FunctionArgumentType.promise:
        returnValue = 'IDLPromise';
        break;
      case FunctionArgumentType.legacy_dom_string:
        // TODO: legacy is now allowed with nullable
        returnValue = 'IDLLegacyDOMString'
        break;
      default:
      case FunctionArgumentType.any:
        returnValue = `IDLAny`;
        break;
    }
  }

  if (haveNull) {
    returnValue = `IDLNullable<${returnValue}>`;
  } else if (isOptional) {
    returnValue = `IDLOptional<${returnValue}>`;
  }

  return returnValue;
}

function isDOMStringType(type: ParameterType) {
  return type.value == FunctionArgumentType.dom_string || type.value == FunctionArgumentType.legacy_dom_string;
}

function generateNativeValueTypeConverter(type: ParameterType): string {
  let returnValue = '';

  if (isPointerType(type)) {
    return `NativeTypePointer<${getPointerType(type)}>`;
  }

  switch (type.value) {
    case FunctionArgumentType.int32:
      returnValue = `NativeTypeInt64`;
      break;
    case FunctionArgumentType.int64:
      returnValue = 'NativeTypeInt64';
      break;
    case FunctionArgumentType.double:
      returnValue = `NativeTypeDouble`;
      break;
    case FunctionArgumentType.boolean:
      returnValue = `NativeTypeBool`;
      break;
    case FunctionArgumentType.dom_string:
    case FunctionArgumentType.legacy_dom_string:
      returnValue = `NativeTypeString`;
      break;
  }

  return returnValue;
}

function generateRequiredInitBody(argument: FunctionArguments, argsIndex: number) {
  let type = generateIDLTypeConverter(argument.type, !argument.required);

  let hasArgumentCheck = type.indexOf('Element') >= 0 || type.indexOf('Node') >= 0 || type === 'EventTarget' || type.indexOf('DOMMatrix') >= 0;

  let body = '';
  if (argument.isDotDotDot) {
    body = `Converter<${type}>::FromValue(ctx, argv + ${argsIndex}, argc - ${argsIndex}, exception_state)`
  } else if (hasArgumentCheck) {
    body = `Converter<${type}>::ArgumentsValue(context, argv[${argsIndex}], ${argsIndex}, exception_state)`;
  } else {
    body = `Converter<${type}>::FromValue(ctx, argv[${argsIndex}], exception_state)`;
  }

  return `auto&& args_${argument.name} = ${body};
if (UNLIKELY(exception_state.HasException())) {
  return exception_state.ToQuickJS();
}`;
}

function generateCallMethodName(name: string) {
  if (name === 'constructor') return 'Create';
  return name;
}

function generateDartImplCallCode(blob: IDLBlob, declare: FunctionDeclaration, isLayoutIndependent: boolean, args: FunctionArguments[]): string {
  let nativeArguments = args.map(i => {
    return `NativeValueConverter<${generateNativeValueTypeConverter(i.type)}>::ToNativeValue(${isDOMStringType(i.type) ? 'ctx, ' : ''}args_${i.name})`;
  });

  let returnValueAssignment = '';

  if (declare.returnType.value != FunctionArgumentType.void) {
    returnValueAssignment = 'auto&& native_value =';
  }

  return `
auto* self = toScriptWrappable<${getClassName(blob)}>(JS_IsUndefined(this_val) ? context->Global() : this_val);
${nativeArguments.length > 0 ? `NativeValue arguments[] = {
  ${nativeArguments.join(',\n')}
}` : 'NativeValue* arguments = nullptr;'};
${returnValueAssignment}self->InvokeBindingMethod(binding_call_methods::k${declare.name}, ${nativeArguments.length}, arguments, FlushUICommandReason::kDependentsOnElement${isLayoutIndependent ? '| FlushUICommandReason::kDependentsOnLayout' : ''}, exception_state);
${returnValueAssignment.length > 0 ? `return Converter<${generateIDLTypeConverter(declare.returnType)}>::ToValue(NativeValueConverter<${generateNativeValueTypeConverter(declare.returnType)}>::FromNativeValue(native_value))` : ''};
  `.trim();
}

function generateOptionalInitBody(blob: IDLBlob, declare: FunctionDeclaration, argument: FunctionArguments, argsIndex: number, previousArguments: string[], options: GenFunctionBodyOptions) {
  let call = '';
  let returnValueAssignment = '';
  if (declare.returnType.value != FunctionArgumentType.void) {
    returnValueAssignment = 'return_value =';
  }
  if (declare.returnTypeMode?.dartImpl) {
    call = generateDartImplCallCode(blob, declare, declare.returnTypeMode?.layoutDependent ?? false, declare.args.slice(0, argsIndex + 1));
  } else if (options.isInstanceMethod) {
    call = `auto* self = toScriptWrappable<${getClassName(blob)}>(JS_IsUndefined(this_val) ? context->Global() : this_val);
${returnValueAssignment} self->${generateCallMethodName(declare.name)}(${[...previousArguments, `args_${argument.name}`, 'exception_state'].join(',')});`;
  } else {
    call = `${returnValueAssignment} ${getClassName(blob)}::${generateCallMethodName(declare.name)}(context, ${[...previousArguments, `args_${argument.name}`].join(',')}, exception_state);`;
  }


  return `auto&& args_${argument.name} = Converter<IDLOptional<${generateIDLTypeConverter(argument.type)}>>::FromValue(ctx, argv[${argsIndex}], exception_state);
if (UNLIKELY(exception_state.HasException())) {
  return exception_state.ToQuickJS();
}

if (argc <= ${argsIndex + 1}) {
  ${call}
  break;
}`;
}

function generateFunctionCallBody(blob: IDLBlob, declaration: FunctionDeclaration, options: GenFunctionBodyOptions = {
  isConstructor: false,
  isInstanceMethod: false
}) {
  if (options.isConstructor && declaration.returnType.value == FunctionArgumentType.void) {
    return 'return JS_ThrowTypeError(ctx, "Illegal constructor");';
  }

  let minimalRequiredArgc = 0;
  declaration.args.forEach(m => {
    if (m.required) minimalRequiredArgc++;
  });

  let requiredArguments: string[] = [];
  let requiredArgumentsInit: string[] = [];
  if (minimalRequiredArgc > 0) {
    requiredArgumentsInit = declaration.args.filter((a, i) => a.required).map((a, i) => {
      requiredArguments.push(`args_${a.name}`);
      return generateRequiredInitBody(a, i);
    });
  }

  let optionalArgumentsInit: string[] = [];
  let totalArguments: string[] = requiredArguments.slice();

  for (let i = minimalRequiredArgc; i < declaration.args.length; i++) {
    optionalArgumentsInit.push(generateOptionalInitBody(blob, declaration, declaration.args[i], i, totalArguments, options));
    totalArguments.push(`args_${declaration.args[i].name}`);
  }

  requiredArguments.push('exception_state');

  let call = '';
  let returnValueAssignment = '';
  if (declaration.returnType.value != FunctionArgumentType.void) {
    returnValueAssignment = 'return_value =';
  }
  if (declaration.returnTypeMode?.dartImpl) {
    call = generateDartImplCallCode(blob, declaration, declaration.returnTypeMode?.layoutDependent ?? false, declaration.args.slice(0, minimalRequiredArgc));
  } else if (options.isInstanceMethod) {
    call = `auto* self = toScriptWrappable<${getClassName(blob)}>(JS_IsUndefined(this_val) ? context->Global() : this_val);
${returnValueAssignment} self->${generateCallMethodName(declaration.name)}(${minimalRequiredArgc > 0 ? `${requiredArguments.join(',')}` : 'exception_state'});`;
  } else {
    call = `${returnValueAssignment} ${getClassName(blob)}::${generateCallMethodName(declaration.name)}(context, ${requiredArguments.join(',')});`;
  }

  let minimalRequiredCall = (declaration.args.length == 0 || (declaration.args[0].isDotDotDot)) ? call : `if (argc <= ${minimalRequiredArgc}) {
  ${call}
  break;
}`;

  return `${requiredArgumentsInit.join('\n')}
${minimalRequiredCall}

${optionalArgumentsInit.join('\n')}
`;
}

function generateOverLoadSwitchBody(overloadMethods: FunctionDeclaration[]) {
  let callBodyList = overloadMethods.map((overload, index) => {
    return `if (${overload.args.length} == argc) {
  return ${overload.name}_overload_${index}(ctx, this_val, argc, argv);
}
    `;
  });

  return `
${callBodyList.join('\n')}

return ${overloadMethods[0].name}_overload_${0}(ctx, this_val, argc, argv);
`;
}

function isJSArrayBuiltInProps(prop: PropsDeclaration) {
  return prop.type.value == FunctionArgumentType.js_array_proto_methods;
}

function generateDictionaryInit(blob: IDLBlob, props: PropsDeclaration[]) {
  let initExpression = props.map(prop => {
    switch (prop.type.value) {
      case FunctionArgumentType.boolean: {
        return `${prop.name}_(false)`;
      }
    }
    return ''
  });

  // Remove empty.
  initExpression = initExpression.filter(i => !!i);

  if (initExpression.length == 0) return '';

  return ': ' + initExpression.join(',');
}

function generateReturnValueInit(blob: IDLBlob, type: ParameterType, options: GenFunctionBodyOptions = {
  isConstructor: false,
  isInstanceMethod: false
}) {
  if (type.value == FunctionArgumentType.void) return '';


  if (options.isConstructor) {
    return `${getClassName(blob)}* return_value = nullptr;`
  }
  if (isUnionType(type) && Array.isArray(type.value)) {
    return `std::shared_ptr<${generateUnionTypeClassName(type.value)}> return_value = nullptr;`;
  }

  if (isPointerType(type)) {
    if (getPointerType(type) === 'Promise') {
      return 'ScriptPromise return_value;';
    } else {
      return `${getPointerType(type)}* return_value = nullptr;`;
    }
  }
  return `Converter<${generateIDLTypeConverter(type)}>::ImplType return_value;`;
}

function generateReturnValueResult(blob: IDLBlob, type: ParameterType, mode?: ParameterMode, options: GenFunctionBodyOptions = {
  isConstructor: false,
  isInstanceMethod: false
}): string {
  if (type.value == FunctionArgumentType.void) return 'JS_NULL';
  let method = 'ToQuickJS';

  if (options.isConstructor) {
    return `return_value->${method}()`;
  }

  return `Converter<${generateIDLTypeConverter(type)}>::ToValue(ctx, std::move(return_value))`;
}

type GenFunctionBodyOptions = { isConstructor?: boolean, isInstanceMethod?: boolean };

function generateFunctionBody(blob: IDLBlob, declare: FunctionDeclaration, options: GenFunctionBodyOptions = {
  isConstructor: false,
  isInstanceMethod: false
}) {
  let paramCheck = generateMethodArgumentsCheck(declare);
  let callBody = generateFunctionCallBody(blob, declare, options);
  let returnValueInit = generateReturnValueInit(blob, declare.returnType, options);
  let returnValueResult = generateReturnValueResult(blob, declare.returnType, declare.returnTypeMode, options);

  let constructorPrototypeInit = (options.isConstructor && returnValueInit.length > 0) ? `JSValue proto = JS_GetPropertyStr(ctx, this_val, "prototype");
  JS_SetPrototype(ctx, return_value->ToQuickJSUnsafe(), proto);
  JS_FreeValue(ctx, proto);` : '';

  return `${paramCheck}

  ExceptionState exception_state;
  ExecutingContext* context = ExecutingContext::From(ctx);
  if (!context->IsContextValid()) return JS_NULL;
  
  context->dartIsolateContext()->profiler()->StartTrackSteps("${getClassName(blob)}::${declare.name}");
  
  MemberMutationScope scope{context};
  ${returnValueInit}

  do {  // Dummy loop for use of 'break'.
${addIndent(callBody, 4)}
  } while (false);
  
   context->dartIsolateContext()->profiler()->FinishTrackSteps();

  if (UNLIKELY(exception_state.HasException())) {
    return exception_state.ToQuickJS();
  }
  ${constructorPrototypeInit}
  return ${returnValueResult};
`;
}

function readTemplate(name: string) {
  return fs.readFileSync(path.join(__dirname, '../../templates/idl_templates/' + name + '.cc.tpl'), {encoding: 'utf-8'});
}

export function generateCppSource(blob: IDLBlob, options: GenerateOptions) {
  const baseTemplate = fs.readFileSync(path.join(__dirname, '../../templates/idl_templates/base.cc.tpl'), {encoding: 'utf-8'});
  const className = getClassName(blob)

  const contents = blob.objects.map(object => {
    const templateKind = getTemplateKind(object);
    if (templateKind === TemplateKind.null) return '';

    switch (templateKind) {
      case TemplateKind.Interface: {
        object = object as ClassObject;

        function addObjectProps(prop: PropsDeclaration) {
          if (prop.isSymbol) {
            options.classMethodsInstallList.push(`{JS_ATOM_${prop.name}, ${prop.name}AttributeGetCallback, ${prop.readonly ? 'nullptr' : `${prop.name}AttributeSetCallback`}}`)
          } else {
            options.classMethodsInstallList.push(`{defined_properties::k${prop.name}.Impl(), ${prop.name}AttributeGetCallback, ${prop.readonly ? 'nullptr' : `${prop.name}AttributeSetCallback`}}`)
          }
        }
        function addObjectMethods(method: FunctionDeclaration, i: number) {
          if (overloadMethods.hasOwnProperty(method.name)) {
            overloadMethods[method.name].push(method)
          } else {
            overloadMethods[method.name] = [method];
            filtedMethods.push(method);
            options.classPropsInstallList.push(`{"${method.name}", qjs_${method.name}, ${method.args.length}}`)
          }
        }

        object.props.forEach(addObjectProps);

        let overloadMethods = {};
        let filtedMethods: FunctionDeclaration[] = [];
        object.methods.forEach(addObjectMethods);

        if (object.construct) {
          options.constructorInstallList.push(`{defined_properties::k${className}.Impl(), nullptr, nullptr, constructor}`)
        }

        let wrapperTypeRegisterList = [
          `JS_CLASS_${getWrapperTypeInfoNameOfClassName(className)}`,                        // ClassId
          `"${className}"`,                                                          // ClassName
          object.parent != null ? `${object.parent}::GetStaticWrapperTypeInfo()` : 'nullptr', // parentClassWrapper
          object.construct ? `QJS${className}::ConstructorCallback` : 'nullptr',     // ConstructorCallback
        ];

        // Generate indexed property callback.
        if (object.indexedProp) {
          if (object.indexedProp.indexKeyType == 'number') {
            wrapperTypeRegisterList.push(`IndexedPropertyGetterCallback`);
            if (!object.indexedProp.readonly) {
              wrapperTypeRegisterList.push(`IndexedPropertySetterCallback`);
            } else {
              wrapperTypeRegisterList.push('nullptr');
            }
            wrapperTypeRegisterList.push('nullptr');
            wrapperTypeRegisterList.push('nullptr');
          } else {
            wrapperTypeRegisterList.push('nullptr');
            wrapperTypeRegisterList.push('nullptr');

            wrapperTypeRegisterList.push(`StringPropertyGetterCallback`);
            if (!object.indexedProp.readonly) {
              wrapperTypeRegisterList.push(`StringPropertySetterCallback`);
            } else {
              wrapperTypeRegisterList.push('nullptr');
            }
          }

          wrapperTypeRegisterList.push('PropertyCheckerCallback');
          wrapperTypeRegisterList.push('PropertyEnumerateCallback');
          if (!object.indexedProp.readonly) {
            wrapperTypeRegisterList.push('StringPropertyDeleterCallback')
          } else {
            wrapperTypeRegisterList.push('nullptr');
          }
        }

        let mixinParent = object.mixinParent;
        let mixinObjects: ClassObject[] | null = null;
        if (mixinParent) {
          mixinObjects = mixinParent.map(mixinName => ClassObject.globalClassMap[mixinName]).filter(o => !!o);

          mixinObjects.forEach(mixinObject => {
            mixinObject.methods.forEach(addObjectMethods);
            mixinObject.props.forEach(addObjectProps);
          });
        }

        options.wrapperTypeInfoInit = `
const WrapperTypeInfo QJS${className}::wrapper_type_info_ {${wrapperTypeRegisterList.join(', ')}};
const WrapperTypeInfo& ${className}::wrapper_type_info_ = QJS${className}::wrapper_type_info_;`;
        return _.template(readTemplate('interface'))({
          className,
          blob: blob,
          object: object,
          mixinObjects,
          generateFunctionBody,
          generateCoreTypeValue,
          generateRawTypeValue,
          generateOverLoadSwitchBody,
          isTypeNeedAllocate,
          overloadMethods,
          isJSArrayBuiltInProps,
          filtedMethods,
          generateIDLTypeConverter,
          generateNativeValueTypeConverter,
          isDOMStringType,
        });
      }
      case TemplateKind.Dictionary: {
        dictionaryClasses.push(className);
        let props = (object as ClassObject).props;
        return _.template(readTemplate('dictionary'))({
          className,
          blob: blob,
          props: props,
          object: object,
          generateIDLTypeConverter,
          generateDictionaryInit
        });
      }
      case TemplateKind.globalFunction: {
        object = object as FunctionObject;
        options.globalFunctionInstallList.push(` {"${object.declare.name}", ${object.declare.name}, ${object.declare.args.length}}`);
        return _.template(readTemplate('global_function'))({
          className,
          blob: blob,
          object: object,
          generateFunctionBody
        });
      }
    }
    return '';
  });

  return _.template(baseTemplate)({
    content: contents.join('\n'),
    className,
    blob: blob,
    ...options
  }).split('\n').filter(str => {
    return str.trim().length > 0;
  }).join('\n');
}

export function generateUnionTypeSource(unionType: ParameterType): string {
  return _.template(readTemplate('union'))({
    unionType,
    generateUnionTypeClassName,
    generateUnionTypeFileName,
    generateTypeRawChecker,
    generateUnionMemberName,
    generateUnionTypeClear,
    generateIDLTypeConverter,
    generateUnionConstructorImpl,
    generateUnionTypeSetter,
    getUnionTypeName,
    isPointerType,
    isTypeHaveNull,
    isTypeHaveString
  }).split('\n').filter(str => {
    return str.trim().length > 0;
  }).join('\n');
}