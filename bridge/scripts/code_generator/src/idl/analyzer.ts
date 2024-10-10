import ts, {HeritageClause, ScriptTarget, VariableStatement} from 'typescript';
import {IDLBlob} from './IDLBlob';
import {
  ClassObject,
  ClassObjectKind,
  FunctionArguments,
  FunctionArgumentType,
  FunctionDeclaration,
  FunctionObject,
  IndexedPropertyDeclaration,
  ParameterMode,
  PropsDeclaration,
} from './declaration';
import {isUnionType} from "./generateSource";

interface DefinedPropertyCollector {
  properties: Set<string>;
  files: Set<string>;
  interfaces: Set<string>;
}

export interface UnionTypeCollector {
  types: Set<ParameterType[]>;
}

export function analyzer(blob: IDLBlob, definedPropertyCollector: DefinedPropertyCollector, unionTypeCollector: UnionTypeCollector) {
  let code = blob.raw;
  const sourceFile = ts.createSourceFile(blob.source, blob.raw, ScriptTarget.ES2020);
  blob.objects = sourceFile.statements.map(statement => walkProgram(blob, statement, definedPropertyCollector, unionTypeCollector)).filter(o => {
    return o instanceof ClassObject || o instanceof FunctionObject;
  }) as (FunctionObject | ClassObject)[];
}

function getInterfaceName(statement: ts.Statement) {
  return (statement as ts.InterfaceDeclaration).name.escapedText;
}

function getHeritageType(heritage: HeritageClause) {
  let expression = heritage.types[0].expression;
  if (expression.kind === ts.SyntaxKind.Identifier) {
    let heritageText = (expression as ts.Identifier).escapedText as string;
    if (heritageText.toLowerCase().indexOf('mixin') >= 0) {
      return null;
    }
    return heritageText;
  }
  return null;
}

function getMixins(hasParent: boolean, hertage: HeritageClause): string[] | null {
  const sliceIndex = (hasParent ? 1 : 0);
  if (hertage.types.length <= sliceIndex) return null;
  let mixins: string[] = [];
  hertage.types.slice(sliceIndex).forEach(types => {
    let expression = types.expression;
    if (expression.kind === ts.SyntaxKind.Identifier) {
      mixins.push((expression as ts.Identifier).escapedText! as string);
    }
  });
  return mixins;
}

function getPropName(propName: ts.PropertyName, prop?: PropsDeclaration) {
  if (propName.kind == ts.SyntaxKind.Identifier) {
    return propName.escapedText.toString();
  } else if (propName.kind === ts.SyntaxKind.StringLiteral) {
    return propName.text;
  } else if (propName.kind === ts.SyntaxKind.NumericLiteral) {
    return propName.text;
    // @ts-ignore
  } else if (propName.kind === ts.SyntaxKind.ComputedPropertyName && propName.expression.kind === ts.SyntaxKind.PropertyAccessExpression) {
    prop!.isSymbol = true;
    // @ts-ignore
    let expression = propName.expression;
    // @ts-ignore
    return `${expression.expression.text}_${expression.name.text}`;
  }
  throw new Error(`prop name: ${ts.SyntaxKind[propName.kind]} is not supported`);
}

function getParameterName(name: ts.BindingName) : string {
  if (name.kind === ts.SyntaxKind.Identifier) {
    return name.escapedText.toString();
  }
  return  '';
}

export type ParameterBaseType = FunctionArgumentType | string;
export type ParameterType = {
  isArray?: boolean;
  value: ParameterType | ParameterType[] | ParameterBaseType;
};

function getParameterBaseType(type: ts.TypeNode, mode?: ParameterMode): ParameterBaseType {
  if (type.kind === ts.SyntaxKind.StringKeyword) {
    return FunctionArgumentType.dom_string;
  } else if (type.kind === ts.SyntaxKind.NumberKeyword) {
    return FunctionArgumentType.double;
  } else if (type.kind === ts.SyntaxKind.BooleanKeyword) {
    return FunctionArgumentType.boolean;
  } else if (type.kind === ts.SyntaxKind.AnyKeyword) {
    return FunctionArgumentType.any;
  } else if (type.kind === ts.SyntaxKind.ObjectKeyword) {
    return FunctionArgumentType.object;
    // @ts-ignore
  } else if (type.kind === ts.SyntaxKind.VoidKeyword) {
    return FunctionArgumentType.void;
  } else if (type.kind === ts.SyntaxKind.NullKeyword) {
    return FunctionArgumentType.null;
  } else if (type.kind === ts.SyntaxKind.UndefinedKeyword) {
    return FunctionArgumentType.undefined;
  } else if (type.kind === ts.SyntaxKind.TypeReference) {
    let typeReference: ts.TypeReference = type as unknown as ts.TypeReference;
    // @ts-ignore
    let identifier = (typeReference.typeName as ts.Identifier).text;
    if (identifier === 'Function') {
      return FunctionArgumentType.function;
    } else if (identifier === 'Promise') {
      return FunctionArgumentType.promise;
    } else if (identifier === 'int32') {
      return FunctionArgumentType.int32;
    } else if (identifier === 'JSArrayProtoMethod') {
      return FunctionArgumentType.js_array_proto_methods;
    } else if (identifier === 'int64') {
      return FunctionArgumentType.int64;
    } else if (identifier === 'double') {
      return FunctionArgumentType.double;
    } else if (identifier === 'NewObject') {
      if (mode) mode.newObject = true;
      let argument = typeReference.typeArguments![0];
      // @ts-ignore
      return argument.typeName.text;
    } else if (identifier === 'DartImpl') {
      if (mode) mode.dartImpl = true;
      let argument: ts.TypeNode = typeReference.typeArguments![0] as unknown as ts.TypeNode;

      if (argument.kind == ts.SyntaxKind.TypeReference) {
        let typeReference: ts.TypeReference = argument as unknown as ts.TypeReference;
        // @ts-ignore
        let identifier = (typeReference.typeName as ts.Identifier).text;

        if (identifier == 'DependentsOnLayout') {
          if (mode) {
            mode.layoutDependent = true;
          }
          argument = typeReference.typeArguments![0] as unknown as ts.TypeNode;
        }
      }

      // @ts-ignore
      return getParameterBaseType(argument);
    } else if (identifier === 'StaticMember') {
      if (mode) mode.static = true;
      let argument = typeReference.typeArguments![0];
      // @ts-ignore
      return getParameterBaseType(argument);
    } else if (identifier === 'LegacyNullToEmptyString') {
      return FunctionArgumentType.legacy_dom_string;
    } else if (identifier === 'ImplementedAs') {
      let secondNameNode: ts.LiteralTypeNode = typeReference.typeArguments![1] as unknown as ts.LiteralTypeNode;
      if (mode) {
        mode.secondaryName = secondNameNode.literal['text'] as string;
      }
      return getParameterBaseType(typeReference.typeArguments![0] as unknown as ts.TypeNode);
    }

    return identifier;
  } else if (type.kind === ts.SyntaxKind.LiteralType) {
    // @ts-ignore
    return getParameterBaseType((type as ts.LiteralTypeNode).literal, mode);
  }

  return FunctionArgumentType.any;
}

function getParameterType(type: ts.TypeNode, unionTypeCollector: UnionTypeCollector, mode?: ParameterMode): ParameterType {
  if (type.kind === ts.SyntaxKind.ParenthesizedType) {
    let typeNode = type as ts.ParenthesizedTypeNode;
    return getParameterType(typeNode.type, unionTypeCollector, mode);
  }

  if (type.kind == ts.SyntaxKind.ArrayType) {
    let arrayType = type as unknown as ts.ArrayTypeNode;
    return {
      isArray: true,
      value: getParameterType(arrayType.elementType, unionTypeCollector, mode)
    };
  } else if (type.kind === ts.SyntaxKind.UnionType) {
    let node = type as unknown as ts.UnionType;
    let types = node.types;
    let result = {
      isArray: false,
      value: types.map(type => getParameterType(type as unknown as ts.TypeNode, unionTypeCollector, mode))
    };
    if (isUnionType(result)) {
      unionTypeCollector.types.add(result.value);
    }
    return result;
  }
  return {
    isArray: false,
    value: getParameterBaseType(type, mode)
  };
}

function paramsNodeToArguments(parameter: ts.ParameterDeclaration, unionTypeCollector: UnionTypeCollector): FunctionArguments {
  let args = new FunctionArguments();
  args.name = getParameterName(parameter.name);
  let typeMode = new ParameterMode();
  args.type = getParameterType(parameter.type!, unionTypeCollector, typeMode);
  args.isDotDotDot = !!parameter.dotDotDotToken;
  args.typeMode = typeMode;
  args.required = !parameter.questionToken;
  return args;
}

function isParamsReadOnly(m: ts.PropertySignature): boolean {
  if (!m.modifiers) return false;
  return m.modifiers.some(k => k.kind === ts.SyntaxKind.ReadonlyKeyword);
}

function walkProgram(blob: IDLBlob, statement: ts.Statement, definedPropertyCollector: DefinedPropertyCollector, unionTypeCollector: UnionTypeCollector) {
  switch(statement.kind) {
    case ts.SyntaxKind.InterfaceDeclaration: {
      let interfaceName = getInterfaceName(statement) as string;
      let s = (statement as ts.InterfaceDeclaration);
      let obj = new ClassObject();
      let constructorDefined = false;
      if (s.heritageClauses) {
        let heritage = s.heritageClauses[0];
        let heritageType = getHeritageType(heritage);
        let mixins = getMixins(heritageType != null, heritage);
        if (heritageType) obj.parent = heritageType.toString();
        if (mixins) obj.mixinParent = mixins;
      }

      obj.name = s.name.escapedText.toString();

      if (s.decorators) {
        let decoratorExpression = s.decorators[0].expression as ts.CallExpression;
        // @ts-ignore
        if (decoratorExpression.expression.kind === ts.SyntaxKind.Identifier && decoratorExpression.expression.escapedText === 'Dictionary') {
          obj.kind = ClassObjectKind.dictionary;
          // @ts-ignore
        } else if (decoratorExpression.expression.kind === ts.SyntaxKind.Identifier && decoratorExpression.expression.escapedText === 'Mixin') {
          obj.kind = ClassObjectKind.mixin;
        }
      }

      if (obj.kind === ClassObjectKind.interface) {
        definedPropertyCollector.interfaces.add('QJS' + interfaceName);
        definedPropertyCollector.files.add(blob.filename);
        definedPropertyCollector.properties.add(interfaceName);
      }

      s.members.forEach(member => {
        switch(member.kind) {
          case ts.SyntaxKind.PropertySignature: {
            let prop = new PropsDeclaration();
            let m = (member as ts.PropertySignature);
            prop.name = getPropName(m.name, prop);
            prop.readonly = isParamsReadOnly(m);

            definedPropertyCollector.properties.add(prop.name);
            let propKind = m.type;
            if (propKind) {
              let mode = new ParameterMode();
              prop.type = getParameterType(propKind, unionTypeCollector, mode);
              prop.typeMode = mode;
              if (member.questionToken) {
                prop.optional = true;
              }
              if (prop.type.value === FunctionArgumentType.function) {
                let f = (m.type as ts.FunctionTypeNode);
                let functionProps = prop as FunctionDeclaration;
                functionProps.args = [];
                f.parameters.forEach(params => {
                  let p = paramsNodeToArguments(params, unionTypeCollector);
                  functionProps.args.push(p);
                });
                obj.methods.push(functionProps);
              } else {
                obj.props.push(prop);
              }
            }
            break;
          }
          case ts.SyntaxKind.MethodSignature: {
            let m = (member as ts.MethodSignature);
            let f = new FunctionDeclaration();
            f.name = getPropName(m.name);
            f.args = [];
            m.parameters.forEach(params => {
              let p = paramsNodeToArguments(params, unionTypeCollector);
              f.args.push(p);
            });
            obj.methods.push(f);
            if (m.type) {
              let mode = new ParameterMode();
              f.returnType = getParameterType(m.type, unionTypeCollector, mode);
              f.returnTypeMode = mode;

              if (mode.secondaryName) {
                f.name = mode.secondaryName;
              }
            }
            break;
          }
          case ts.SyntaxKind.IndexSignature: {
            let m = (member as ts.IndexSignatureDeclaration);
            let prop = new IndexedPropertyDeclaration();
            let modifier = m.modifiers;
            prop.readonly = !!(modifier && modifier[0].kind == ts.SyntaxKind.ReadonlyKeyword);

            let params = m.parameters;
            prop.indexKeyType = params[0].type!.kind === ts.SyntaxKind.NumberKeyword ? 'number' : 'string';

            let mode = new ParameterMode();
            prop.type = getParameterType(m.type, unionTypeCollector, mode);
            prop.typeMode = mode;
            obj.indexedProp = prop;
            break;
          }
          case ts.SyntaxKind.ConstructSignature: {
            let m = (member as unknown as ts.ConstructorTypeNode);
            let c = new FunctionDeclaration();
            c.name = 'constructor';
            c.args = [];
            m.parameters.forEach(params => {
              let p = paramsNodeToArguments(params, unionTypeCollector);
              c.args.push(p);
            });
            c.returnType = getParameterType(m.type, unionTypeCollector);
            obj.construct = c;
            constructorDefined = true;
            break;
          }
        }
      });

      if (!constructorDefined && obj.kind === ClassObjectKind.interface) {
        throw new Error(`Interface: ${interfaceName} didn't have constructor defined.`);
      }

      ClassObject.globalClassMap[interfaceName] = obj;

      return obj;
    }
    case ts.SyntaxKind.VariableStatement: {
      let declaration = (statement as VariableStatement).declarationList.declarations[0];
      let methodName = (declaration.name as ts.Identifier).text;
      let type = declaration.type;
      let functionObject = new FunctionObject();

      functionObject.declare = new FunctionDeclaration();
      if (type?.kind == ts.SyntaxKind.FunctionType) {
        functionObject.declare.args = (type as ts.FunctionTypeNode).parameters.map(param => paramsNodeToArguments(param, unionTypeCollector));
        functionObject.declare.returnType = getParameterType((type as ts.FunctionTypeNode).type, unionTypeCollector);
        functionObject.declare.name = methodName.toString();
      }

      return functionObject;
    }
  }

  return null;
}
