import 'package:flutter/material.dart';
import 'package:flutter_mobx/flutter_mobx.dart';

class SettingsTextListRow extends StatelessWidget {
  SettingsTextListRow({@required this.onTaped, this.title, this.widget});

  final VoidCallback onTaped;
  final String title;
  final Widget widget;

  @override
  Widget build(BuildContext context) {
    return Container(
      color: Theme.of(context).accentTextTheme.headline5.backgroundColor,
      child: ListTile(
        contentPadding: EdgeInsets.only(left: 20.0, right: 20.0),
        title: Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: <Widget>[
            Flexible(
              child: Observer(
                  builder: (_) => Text(
                        title,
                        style: TextStyle(
                            fontSize: 16.0,
                            color: Theme.of(context)
                                .primaryTextTheme
                                .headline6
                                .color),
                      )),
            ),
            Flexible(child: widget)
          ],
        ),
        onTap: onTaped,
      ),
    );
  }
}
