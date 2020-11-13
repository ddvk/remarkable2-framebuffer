#include "../shared/swtfb.cpp"

#include <cstdio>
#include <iostream>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <stdint.h>

void dump_qtClass(QObject* object) {
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

extern "C" {
static void _libhook_init() __attribute__((constructor));
static void _libhook_init() { printf("LIBHOOK INIT\n"); }

int main(int, char **, char **) {
  swtfb::SwtFB fb;
    while(true) {
        cout << "Waiting: " << endl;
        std::cin.ignore();
        fb.FullScreen(0xFF);
        cout << "again" << endl;
        fb.DrawLine();

    }

  printf("END of our main\n");
}

int __libc_start_main(int (*_main)(int, char **, char **), int argc,
                      char **argv, int (*init)(int, char **, char **),
                      void (*fini)(void), void (*rtld_fini)(void),
                      void *stack_end) {

  printf("LIBC START HOOK\n");

  typeof(&__libc_start_main) func_main =
      (typeof(&__libc_start_main))dlsym(RTLD_NEXT, "__libc_start_main");

  return func_main(main, argc, argv, init, fini, rtld_fini, stack_end);
};
};
