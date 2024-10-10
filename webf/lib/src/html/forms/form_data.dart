import 'dart:typed_data';
import 'package:webf/foundation.dart';

class FormData extends DynamicBindingObject {
  FormData(BindingContext context, List<dynamic> domMatrixInit): super(context);

  @override
  void initializeMethods(Map<String, BindingObjectMethod> methods) {
    methods['append'] = BindingObjectMethodSync(call: (args) {
      Uint8List bytes = args[0];
      print('bytes: $bytes');
    });
  }

  @override
  void initializeProperties(Map<String, BindingObjectProperty> properties) {
  }
}
