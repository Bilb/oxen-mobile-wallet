import 'package:flutter/material.dart';

class SettingRawWidgetListRow extends StatelessWidget {
  SettingRawWidgetListRow({this.widgetBuilder});

  final WidgetBuilder? widgetBuilder;

  @override
  Widget build(BuildContext context) {
    return Container(
      color: Theme.of(context).accentTextTheme.headlineSmall?.backgroundColor,
      child: widgetBuilder?.call(context) ?? Container(),
    );
  }
}
