/*
 * Copyright (C) 2019-2022 The Kraken authors. All rights reserved.
 * Copyright (C) 2022-present The WebF authors. All rights reserved.
 */

import 'package:flutter/material.dart';
import 'package:webf/webf.dart';
import 'package:webf/devtools.dart';

void main() {
  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Kraken Browser',
      // theme: ThemeData.dark(),
      debugShowCheckedModeBanner: false,
      home: FirstPage(title: 'Landing Bay'),
    );
  }
}

class FirstPage extends StatefulWidget {
  const FirstPage({Key? key, required this.title}) : super(key: key);
  final String title;

  @override
  State<StatefulWidget> createState() {
    return FirstPageState();
  }
}

class FirstPageState extends State<FirstPage> {
  late WebFController controller;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    controller = WebFController(
      context,
      isDarkMode: MediaQuery.of(context).platformBrightness == Brightness.dark,
      devToolsService: ChromeDevToolsService(),
    );
    controller.preload(WebFBundle.fromUrl('assets:assets/bundle.html'), viewportSize: MediaQuery.of(context).size);
  }

  @override
  void dispose() {
    super.dispose();
    controller.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.title),
      ),
      body: Center(
        child: ElevatedButton(
          onPressed: () {
            Navigator.push(context, MaterialPageRoute(builder: (context) {
              return WebFDemo(controller: controller);
            }));
          },
          child: const Text('Open WebF Page'),
        ),
      ),
    );
  }
}

class WebFDemo extends StatelessWidget {
  final WebFController controller;

  WebFDemo({ required this.controller });

  @override
  Widget build(BuildContext context) {
    return Scaffold(
        appBar: AppBar(
          title: Text('WebF Demo'),
        ),
        body: Center(
          // Center is a layout widget. It takes a single child and positions it
          // in the middle of the parent.
          child: WebF(controller: controller),
        ));
  }
}
