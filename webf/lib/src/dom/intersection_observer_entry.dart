/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

import 'dart:ffi';
import 'package:webf/bridge.dart';
import 'element.dart';

class DartIntersectionObserverEntry {
  //final DOMHighResTimeStamp time;
  //final DOMRectReadOnly? rootBounds;
  //final DOMRectReadOnly boundingClientRect;
  //final DOMRectReadOnly intersectionRect;
  final bool isIntersecting;

  //final bool isVisible;
  final double intersectionRatio;
  final Element element;

  DartIntersectionObserverEntry(this.isIntersecting, this.intersectionRatio, this.element);

  DartIntersectionObserverEntry copy() {
    return DartIntersectionObserverEntry(isIntersecting, intersectionRatio, element);
  }
}

class NativeIntersectionObserverEntry extends Struct {
  @Int8()
  external int isIntersecting;

  @Double()
  external double intersectionRatio;

  external Pointer<NativeBindingObject> element;
}
