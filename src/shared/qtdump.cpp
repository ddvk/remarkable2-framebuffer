#pragma once
#include <QObject>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QMetaObject>
#include <QDebug>

void dump_qtClass(void* ptr) {
  printf("INSTANCE ADDRESS: 0x%lx\n", ptr);
  QObject *object = static_cast<QObject *>((QObject*) ptr);
  const QMetaObject *meta = object->metaObject();
  qDebug() << meta->className();

  qDebug() << "Methods";
  for (int id = 0; id < meta->methodCount(); ++id) {
    const QMetaMethod method = meta->method(id);
    if (method.methodType() == QMetaMethod::Slot && method.access() == QMetaMethod::Public)
      qDebug() << method.access() << method.name() << method.parameterNames() << method.parameterTypes();
  }

  qDebug() << "Properties";
  for (int id = 0; id < meta->propertyCount(); ++id) {
    const QMetaProperty property = meta->property(id);
    qDebug() << property.name() << property.type();
  }
}

