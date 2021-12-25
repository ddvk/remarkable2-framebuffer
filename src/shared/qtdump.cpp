#pragma once

#include <QObject>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QMetaObject>
#include <QDebug>

void dump_qtClass(void* ptr) {
  printf("INSTANCE ADDRESS: 0x%lx\n", (long unsigned int)ptr);
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

  qDebug() << "Enumerators";
  for (int id = 0; id < meta->enumeratorCount(); ++id) {
    const QMetaEnum en = meta->enumerator(id);
    qDebug() << en.name();
    for (int j = 0; j < en.keyCount(); j++) {
      qDebug() << en.key(j);
    }
    qDebug() << "";
  }
}

