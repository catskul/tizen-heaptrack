#ifndef STACKENTRY_H
#define STACKENTRY_H

class StackEntry {

public:

  StackEntry(unsigned int funcId, char* className, char* methodName, StackEntry *next)
  : funcId(funcId),
    className(className),
    methodName(methodName),
    next(next)
  {
  }

  ~StackEntry() {
    delete[] className;
    delete[] methodName;
  }

  unsigned int funcId;
  char* className;
  char* methodName;
  StackEntry * next;
};

#endif // STACKENTRY_H
