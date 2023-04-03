/*
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */
import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'package:flutter/foundation.dart';
import 'package:image_compare/image_compare.dart';

import 'package:image/image.dart';
import 'package:path/path.dart' as path;

///Check if [firstImg] and [secondImg] have the same width and height
bool haveSameSize(Image firstImg, Image secondImg) {
  if (firstImg.width != secondImg.width ||
      firstImg.height != secondImg.height) {
    return false;
  }
  return true;
}

///Returns a red color only if two RGB pixels are different
int selectColor(firstPixel, secondPixel, diffAtPixel) {
  var fRed = getRed(firstPixel);
  var fGreen = getGreen(firstPixel);
  var fBlue = getBlue(firstPixel);
  var sRed = getRed(secondPixel);
  var sGreen = getGreen(secondPixel);
  var sBlue = getBlue(secondPixel);

  if (diffAtPixel == 0) return Color.fromRgba(fRed, fGreen, fBlue, 50);
  if (fRed == 0 && fGreen == 0 && fBlue == 0)
    return Color.fromRgba(sRed, sGreen, sBlue, 50);
  if (sRed == 0 && sGreen == 0 && sBlue == 0)
    return Color.fromRgba(fRed, fGreen, fBlue, 50);

  int alpha, red, green, blue;

  alpha = 255;
  red = 255;
  green = 0;
  blue = 0;

  return Color.fromRgba(red, green, blue, alpha);
}

num diffBetweenPixels(firstPixel, secondPixel, ignoreAlpha) {
  var fRed = getRed(firstPixel);
  var fGreen = getGreen(firstPixel);
  var fBlue = getBlue(firstPixel);
  var fAlpha = getAlpha(firstPixel);
  var sRed = getRed(secondPixel);
  var sGreen = getGreen(secondPixel);
  var sBlue = getBlue(secondPixel);
  var sAlpha = getAlpha(secondPixel);

  num diff =
      (fRed - sRed).abs() + (fGreen - sGreen).abs() + (fBlue - sBlue).abs();

  if (ignoreAlpha) {
    diff = (diff / 255) / 3;
  } else {
    diff += (fAlpha - sAlpha).abs();
    diff = (diff / 255) / 4;
  }

  return diff;
}

Future<bool> matchImage(Uint8List imageA, List<int> imageB, String filename) async {
  if (imageA.length == 0 || imageB.length == 0) {
    return false;
  }

  Image a = decodeImage(imageA.toList())!;
  Image b = decodeImage(imageB.toList())!;
  if (!haveSameSize(a, b)) {
    return false;
  }

  double ratio = await compareImages(src1: a, src2: b, algorithm: EuclideanColorDistance(ignoreAlpha: true));
  bool isMatch = (ratio * 100) < 1;

  if (!isMatch) {
    var width = a.width;
    var height = b.height;

    //Create an image to show the differences
    var diffImg = Image(width, height);

    for (var i = 0; i < width; i++) {
      var diffAtPixel, firstPixel, secondPixel;
      for (var j = 0; j < height; j++) {
        firstPixel = a.getPixel(i, j);
        secondPixel = b.getPixel(i, j);

        diffAtPixel = diffBetweenPixels(firstPixel, secondPixel, true);

        //Shows in red the different pixels and in semitransparent the same ones
        diffImg.setPixel(i, j, selectColor(firstPixel, secondPixel, diffAtPixel));
      }
    }

    final newSnap = File('$filename.diff.png');
    newSnap.writeAsBytesSync(encodePng(diffImg));
  }

  return isMatch; // < 0.01%
}

bool matchFile(List<int> left, List<int> right) {
  if (left.length != right.length) {
    return false;
  }

  for (int i = 0; i < left.length; i++) {
    if (left[i] != right[i]) {
      return false;
    }
  }

  return true;
}

Future<bool> matchImageSnapshot(Uint8List bytes, String filename) async {
  final dirname = path.dirname(filename);
  var dir = Directory(dirname);
  if (!dir.existsSync()) {
    dir.createSync(recursive: true);
  }
  List<int> currentPixels = bytes.toList();
  final snap = File('$filename.png');
  if (snap.existsSync()) {
    Uint8List snapPixels = snap.readAsBytesSync();
    bool match;
    if (snapPixels.isEmpty || currentPixels.isEmpty) {
      match = false;
    } else {
      match = matchFile(snapPixels, currentPixels);
    }
    if (!match) {
      match = await matchImage(snapPixels, currentPixels, filename);
    }
    if (!match) {
      final newSnap = File('$filename.current.png');
      newSnap.writeAsBytes(currentPixels);
    }
    return match;
  } else {
    if (Platform.environment['CI'] == 'true') {
      throw FlutterError('This specs did not have corresponding snapshots.');
    }

    await snap.writeAsBytes(currentPixels);
    return true;
  }
}
