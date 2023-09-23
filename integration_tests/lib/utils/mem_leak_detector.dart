import 'dart:math';

double linearRegressionSlope(List<double> x, List<double> y) {
  int n = x.length;
  double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;

  for (int i = 0; i < n; i++) {
    sumX += x[i];
    sumY += y[i];
    sumXY += x[i] * y[i];
    sumX2 += x[i] * x[i];
  }

  return (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
}

bool memoryIncreaseRatio(List<double> memoryUsage) {
  if (memoryUsage.length < 2) return false;

  int increasingCount = 0;
  for (int i = 1; i < memoryUsage.length; i++) {
    if (memoryUsage[i] > memoryUsage[i - 1]) {
      increasingCount++;
    }
  }

  double ratio = increasingCount / memoryUsage.length;
  return ratio > 0.75;
}


bool detectMemoryLeakBasedOnRegression(List<double> memoryUsage) {
  List<double> timePoints = List.generate(memoryUsage.length, (index) => index.toDouble()); // [0.0, 1.0, 2.0, ...]
  double slope = linearRegressionSlope(timePoints, memoryUsage);
  return slope > 10;
}

bool isMemLeaks(List<double> mems) {
  return detectMemoryLeakBasedOnRegression(mems) && memoryIncreaseRatio(mems);
}
