/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
import 'dart:math';
import 'dart:typed_data';

import 'package:flutter/cupertino.dart';
import 'package:vector_math/vector_math_64.dart';
import 'package:webf/bridge.dart';
import 'package:webf/foundation.dart';
import 'package:webf/geometry.dart';
import 'package:webf/src/css/matrix.dart';
import 'package:webf/src/geometry/dom_point.dart';

class DOMMatrixReadOnly extends DynamicBindingObject {
  // Matrix4 Values are stored in column major order.
  Matrix4 _matrix4 = Matrix4.identity();

  Matrix4 get matrix => _matrix4;
  bool _is2D = true;

  bool get is2D => _is2D;

  DOMMatrixReadOnly.fromMatrix4(BindingContext context, Matrix4? matrix4, bool flag2D) : super(context) {
    if (matrix4 != null) {
      _matrix4 = matrix4;
      _is2D = flag2D;
    } else {
      _matrix4 = Matrix4.zero();
      _is2D = false;
    }
  }

  DOMMatrixReadOnly(BindingContext context, List<dynamic> domMatrixInit) : super(context) {
    if (!domMatrixInit.isNotEmpty) {
      return;
    }
    if (domMatrixInit.length == 1) {
      // List<double>
      if (domMatrixInit[0].runtimeType == List<dynamic>) {
        List<dynamic> list = domMatrixInit[0];
        if (list.isNotEmpty && list[0].runtimeType == double) {
          List<double> doubleList = List<double>.from(list);
          if (doubleList.length == 6) {
            _matrix4[0] = doubleList[0]; // m11 = a
            _matrix4[1] = doubleList[1]; // m12 = b
            _matrix4[4] = doubleList[2]; // m21 = c
            _matrix4[5] = doubleList[3]; // m22 = d
            _matrix4[12] = doubleList[4]; // m41 = e
            _matrix4[13] = doubleList[5]; // m42 = f
          } else if (doubleList.length == 16) {
            _is2D = false;
            _matrix4 = Matrix4.fromList(doubleList);
          } else {
            throw TypeError();
          }
        }
      }
    } else if (domMatrixInit.length == 3) {
      // List<double>, is2D, isIdentity
      if (domMatrixInit[0].runtimeType == List<dynamic>) {
        List<dynamic> list = domMatrixInit[0];
        if (list.isNotEmpty && list[0].runtimeType == double) {
          List<double> doubleList = List<double>.from(list);
          if (doubleList.length == 16) {
            _matrix4 = Matrix4.fromList(doubleList);
          } else {
            throw TypeError();
          }
        }
        _is2D = castToType<bool>(domMatrixInit[1]);
        //TODO isIdentity
        // _isIdentity = castToType<bool>(domMatrixInit[2]);
      }
    }
  }

  @override
  void initializeMethods(Map<String, BindingObjectMethod> methods) {
    methods['flipX'] = BindingObjectMethodSync(call: (_) => flipX());
    methods['flipY'] = BindingObjectMethodSync(call: (_) => flipY());
    methods['inverse'] = BindingObjectMethodSync(call: (_) => inverse());
    methods['multiply'] = BindingObjectMethodSync(call: (args) {
      BindingObject domMatrix = args[0];
      if (domMatrix is DOMMatrix) {
        return multiply((domMatrix as DOMMatrix));
      }
    });
    methods['rotateAxisAngle'] = BindingObjectMethodSync(
      call: (args) => rotateAxisAngle(
        castToType<num>(args[0]).toDouble(),
        castToType<num>(args[1]).toDouble(),
        castToType<num>(args[2]).toDouble(),
        castToType<num>(args[3]).toDouble()
      )
    );
    methods['rotate'] = BindingObjectMethodSync(call: (args) {
      if (args.length == 1) {
        // rotate(x)
        return rotateZ(castToType<num>(args[0]).toDouble());
      } else if (args.length == 3) {
        // rotate(x,y,z)
        return rotate(
            castToType<num>(args[0]).toDouble(),
            castToType<num>(args[1]).toDouble(),
            castToType<num>(args[2]).toDouble());
      }
    });

    methods['rotateFromVector'] = BindingObjectMethodSync(
      call: (args) => rotateFromVector(
        castToType<num>(args[0]).toDouble(),
        castToType<num>(args[1]).toDouble()
      )
    );
    methods['scale'] = BindingObjectMethodSync(
      call: (args) => scale(
        castToType<num>(args[0]).toDouble(),
        castToType<num>(args[1]).toDouble(),
        castToType<num>(args[2]).toDouble(),
        castToType<num>(args[3]).toDouble(),
        castToType<num>(args[4]).toDouble(),
        castToType<num>(args[5]).toDouble()
      )
    );
    methods['scale3d'] = BindingObjectMethodSync(
      call: (args) => scale3d(
        castToType<num>(args[0]).toDouble(),
        castToType<num>(args[1]).toDouble(),
        castToType<num>(args[2]).toDouble(),
        castToType<num>(args[3]).toDouble(),
      )
    );
    methods['scaleNonUniform'] = BindingObjectMethodSync(
      call: (args) => scaleNonUniform(
        castToType<num>(args[0]).toDouble(),
        castToType<num>(args[1]).toDouble()
      )
    );
    methods['scale3d'] = BindingObjectMethodSync(
      call: (args) => scale3d(
        castToType<num>(args[0]).toDouble(),
        castToType<num>(args[1]).toDouble(),
        castToType<num>(args[2]).toDouble(),
        castToType<num>(args[3]).toDouble(),
      )
    );
    methods['skewX'] = BindingObjectMethodSync(call: (args) => skewX(castToType<num>(args[0]).toDouble()));
    methods['skewY'] = BindingObjectMethodSync(call: (args) => skewY(castToType<num>(args[0]).toDouble()));
    // toFloat32Array(): number[];
    // toFloat64Array(): number[];
    // toJSON(): DartImpl<JSON>;
    methods['toString'] = BindingObjectMethodSync(call: (args) => toString());
    methods['transformPoint'] = BindingObjectMethodSync(call: (args) {
      BindingObject domPoint = args[0];
      if (domPoint is DOMPoint) {
        return transformPoint(domPoint);
      }
    });
    methods['translate'] = BindingObjectMethodSync(
      call: (args) => translate(
        castToType<num>(args[0]).toDouble(),
        castToType<num>(args[1]).toDouble(),
        castToType<num>(args[2]).toDouble()
      )
    );
  }

  @override
  void initializeProperties(Map<String, BindingObjectProperty> properties) {
    properties['is2D'] = BindingObjectProperty(getter: () => _is2D);
    properties['isIdentity'] = BindingObjectProperty(getter: () => _matrix4.isIdentity());
    // m11 = a
    properties['m11'] = BindingObjectProperty(getter: () {
      return _matrix4[0];
    }, setter: (value) {
      if (value is double) {
        _matrix4[0] = value;
      }
    });
    properties['a'] = BindingObjectProperty(
        getter: () => _matrix4[0],
        setter: (value) {
          if (value is double) {
            _matrix4[0] = value;
          }
        });
    // m12 = b
    properties['m12'] = BindingObjectProperty(
        getter: () => _matrix4[1],
        setter: (value) {
          if (value is double) {
            _matrix4[1] = value;
          }
        });
    properties['b'] = BindingObjectProperty(
        getter: () => _matrix4[1],
        setter: (value) {
          if (value is double) {
            _matrix4[1] = value;
          }
        });
    properties['m13'] = BindingObjectProperty(
        getter: () => _matrix4[2],
        setter: (value) {
          if (value is double) {
            _matrix4[2] = value;
          }
        });
    properties['m14'] = BindingObjectProperty(
        getter: () => _matrix4[3],
        setter: (value) {
          if (value is double) {
            _matrix4[3] = value;
          }
        });

    // m22 = c
    properties['m21'] = BindingObjectProperty(
        getter: () => _matrix4[4],
        setter: (value) {
          if (value is double) {
            _matrix4[4] = value;
          }
        });
    properties['c'] = BindingObjectProperty(
        getter: () => _matrix4[4],
        setter: (value) {
          if (value is double) {
            _matrix4[4] = value;
          }
        });
    // m22 = d
    properties['m22'] = BindingObjectProperty(
        getter: () => _matrix4[5],
        setter: (value) {
          if (value is double) {
            _matrix4[5] = value;
          }
        });
    properties['d'] = BindingObjectProperty(
        getter: () => _matrix4[5],
        setter: (value) {
          if (value is double) {
            _matrix4[5] = value;
          }
        });
    properties['m23'] = BindingObjectProperty(
        getter: () => _matrix4[6],
        setter: (value) {
          if (value is double) {
            _matrix4[6] = value;
          }
        });
    properties['m24'] = BindingObjectProperty(
        getter: () => _matrix4[7],
        setter: (value) {
          if (value is double) {
            _matrix4[7] = value;
          }
        });

    properties['m31'] = BindingObjectProperty(
        getter: () => _matrix4[8],
        setter: (value) {
          if (value is double) {
            _matrix4[8] = value;
          }
        });
    properties['m32'] = BindingObjectProperty(
        getter: () => _matrix4[9],
        setter: (value) {
          if (value is double) {
            _matrix4[9] = value;
          }
        });
    properties['m33'] = BindingObjectProperty(
        getter: () => _matrix4[10],
        setter: (value) {
          if (value is double) {
            _matrix4[10] = value;
          }
        });
    properties['m34'] = BindingObjectProperty(
        getter: () => _matrix4[11],
        setter: (value) {
          if (value is double) {
            _matrix4[11] = value;
          }
        });

    // m41 = e
    properties['m41'] = BindingObjectProperty(
        getter: () => _matrix4[12],
        setter: (value) {
          if (value is double) {
            _matrix4[12] = value;
          }
        });
    properties['e'] = BindingObjectProperty(
        getter: () => _matrix4[12],
        setter: (value) {
          if (value is double) {
            _matrix4[12] = value;
          }
        });
    // m42 = f
    properties['m42'] = BindingObjectProperty(
        getter: () => _matrix4[13],
        setter: (value) {
          if (value is double) {
            _matrix4[13] = value;
          }
        });
    properties['f'] = BindingObjectProperty(
        getter: () => _matrix4[13],
        setter: (value) {
          if (value is double) {
            _matrix4[13] = value;
          }
        });
    properties['m43'] = BindingObjectProperty(
        getter: () => _matrix4[14],
        setter: (value) {
          if (value is double) {
            _matrix4[14] = value;
          }
        });
    properties['m44'] = BindingObjectProperty(
        getter: () => _matrix4[15],
        setter: (value) {
          if (value is double) {
            _matrix4[15] = value;
          }
        });
  }

  DOMMatrix flipX() {
    Matrix4 m = Matrix4.identity()..setEntry(0, 0, -1);
    return DOMMatrix.fromMatrix4(
        BindingContext(ownerView, ownerView.contextId, allocateNewBindingObject()), _matrix4 * m, _is2D);
  }

  DOMMatrix flipY() {
    Matrix4 m = Matrix4.identity()..setEntry(1, 1, -1);
    return DOMMatrix.fromMatrix4(
        BindingContext(ownerView, ownerView.contextId, allocateNewBindingObject()), _matrix4 * m, _is2D);
  }

  DOMMatrix inverse() {
    Matrix4 m = Matrix4.inverted(_matrix4);
    return DOMMatrix.fromMatrix4(BindingContext(ownerView, ownerView.contextId, allocateNewBindingObject()), m, _is2D);
  }

  DOMMatrix multiply(DOMMatrix domMatrix) {
    Matrix4 m = _matrix4.multiplied(domMatrix.matrix);
    return DOMMatrix.fromMatrix4(
        BindingContext(ownerView, ownerView.contextId, allocateNewBindingObject()), m, _is2D);
  }

  DOMMatrix rotateAxisAngle(double x, double y, double z, double angle) {
    Matrix4 m = DOMMatrixReadOnly.rotate3d(x, y, z, angle);
    bool flag2D = _is2D;
    if (x != 0 || y != 0) {
      flag2D = false;
    }
    return DOMMatrix.fromMatrix4(
        BindingContext(ownerView, ownerView.contextId, allocateNewBindingObject()), _matrix4 * m, flag2D);
  }

  DOMMatrix rotateZ(double x) {
    // rotate(-90) => rotateZ(-90)
    double xRad = x * degrees2Radians;
    Matrix4 m = _matrix4.clone()..rotateZ(xRad);
    return DOMMatrix.fromMatrix4(BindingContext(ownerView, ownerView.contextId, allocateNewBindingObject()), m, _is2D);
  }

  DOMMatrix rotate(double x, double y, double z) {
    Matrix4 m = DOMMatrixReadOnly.rotate3d(x, y, z, 0);
    bool flag2D = _is2D;
    if (x != 0 || y == 0) {
      flag2D = false;
    }
    return DOMMatrix.fromMatrix4(BindingContext(ownerView, ownerView.contextId, allocateNewBindingObject()), _matrix4 * m, flag2D);
  }

  DOMMatrix rotateFromVector(double x, double y) {
    Matrix4 m = _matrix4.clone();
    double rad = atan2(y, x);
    double angle = rad * radians2Degrees;
    if (angle % 360 != 0) {
      m.rotateZ(rad);
    }
    return DOMMatrix.fromMatrix4(BindingContext(ownerView, ownerView.contextId, allocateNewBindingObject()), m, _is2D);
  }

  DOMMatrix scale(double sX, double sY, double sZ, double oriX, double oriY, double oriZ) {
    Matrix4 m = _matrix4.clone()
      ..translate(oriX, oriY, oriZ)
      ..scale(sX, sY, sZ)
      ..translate(-oriX, -oriY, -oriZ);
    bool flag2D = _is2D;
    if (sZ != 1 || oriZ != 0) {
      flag2D = false;
    }
    return DOMMatrix.fromMatrix4(BindingContext(ownerView, ownerView.contextId, allocateNewBindingObject()), m, flag2D);
  }

  DOMMatrix scale3d(double scale, double oriX, double oriY, double oriZ) {
    return this.scale(scale, scale, scale, oriX, oriY, oriZ);
  }

  DOMMatrix scaleNonUniform(double sX, double sY) {
    Matrix4 m = _matrix4.clone()..scale(sX, sY, 1);
    return DOMMatrix.fromMatrix4(BindingContext(ownerView, ownerView.contextId, allocateNewBindingObject()), m, _is2D);
  }

  DOMMatrix skewX(double sx) {
    Matrix4 m = Matrix4.skewX(sx * degrees2Radians);
    return DOMMatrix.fromMatrix4(
        BindingContext(ownerView, ownerView.contextId, allocateNewBindingObject()), _matrix4 * m, _is2D);
  }

  DOMMatrix skewY(double sy) {
    Matrix4 m = Matrix4.skewY(sy * degrees2Radians);
    return DOMMatrix.fromMatrix4(
        BindingContext(ownerView, ownerView.contextId, allocateNewBindingObject()), _matrix4 * m, _is2D);
  }

  @override
  String toString() {
    if (_is2D) {
      // a,b,c,d,e,f
      return 'matrix(${_matrix4[0]},${_matrix4[1]},${_matrix4[4]},${_matrix4[5]},${_matrix4[12]},${_matrix4[13]})';
    } else {
      return 'matrix3d(${_matrix4.storage.join(',')})';
    }
  }

  DOMPoint transformPoint(DOMPoint domPoint) {
    double x = domPoint.x, y = domPoint.y, z = domPoint.z, w = domPoint.w;
    if (isIdentityOrTranslation(_matrix4)) {
      x += _matrix4[12];
      y += _matrix4[13];
      z += _matrix4[14];
    } else {
      // Multiply a homogeneous point by a matrix and return the transformed point
      // like method v4MulPointByMatrix(v,m) in WebKit TransformationMatrix
      List input = [x, y, z, w];
      x = dot(input, _matrix4.row0);
      y = dot(input, _matrix4.row1);
      z = dot(input, _matrix4.row2);
      w = dot(input, _matrix4.row3);
    }

    List<dynamic> list = [x, y, z, w];
    return DOMPoint(BindingContext(ownerView, ownerView.contextId, allocateNewBindingObject()), list);
  }

  DOMMatrix translate(double tx, double ty, double tz) {
    Matrix4 m = _matrix4.clone()..translate(tx, ty, tz);
    bool flag2D = _is2D;
    if (tz != 0) {
      flag2D = false;
    }
    return DOMMatrix.fromMatrix4(
        BindingContext(ownerView, ownerView.contextId, allocateNewBindingObject()), m, flag2D);
  }

  static bool isIdentityOrTranslation(Matrix4 matrix) {
    return matrix[0] == 1 && matrix[1] == 0 && matrix[2] == 0 && matrix[3] == 0 &&
      matrix[4] == 0 && matrix[5] == 1 && matrix[6] == 0 && matrix[7] == 0 &&
      matrix[8] == 0 && matrix[9] == 0 && matrix[10] == 1 && matrix[11] == 0 &&
      matrix[15] == 1;
  }

  static Matrix4 rotate3d(double x, double y, double z, double angle) {
    // normalizing x,y,z
    List<double> vec = [x,y,z];
    List<double> norVec = normalize(vec);
    double nx = norVec[0];
    double ny = norVec[1];
    double nz = norVec[2];

    // The 3D rotation matrix is described in CSS Transforms with alpha.
    // Please see: https://drafts.csswg.org/css-transforms-2/#Rotate3dDefined
    double alpha_in_radians = degrees2Radians * (angle / 2);
    double sc = sin (alpha_in_radians) * cos(alpha_in_radians);
    double sq = sin(alpha_in_radians) * sin(alpha_in_radians);

    double m11 = 1 - 2 * (ny * ny + nz * nz) * sq;
    double m12 = 2 * (nx * ny * sq + nz * sc);
    double m13 = 2 * (nx * nz * sq - ny * sc);
    double m14 = 0;
    double m21 = 2 * (nx * ny * sq - nz * sc);
    double m22 = 1 - 2 * (nx * nx + nz * nz) * sq;
    double m23 = 2 * (ny * nz * sq + nx * sc);
    double m24 = 0;
    double m31 = 2 * (nx * nz * sq + ny * sc);
    double m32 = 2 * (ny * nz * sq - nx * sc);
    double m33 = 1 - 2 * (nx * nx + ny * ny) * sq;
    double m34 = 0;
    double m41 = 0;
    double m42 = 0;
    double m43 = 0;
    double m44 = 1;

    return new Matrix4(m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34, m41, m42, m43, m44);
  }
}
