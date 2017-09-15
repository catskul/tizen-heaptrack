#ifndef STACKENTRY_H
#define STACKENTRY_H

#include <string.h>

static constexpr size_t MAX_NAME_LENGTH = 512;

class StackEntry {

public:
  StackEntry(unsigned int funcId, char* className, char* methodName, StackEntry *next);

  unsigned int m_funcId;
  char m_className[MAX_NAME_LENGTH + 1];
  char m_methodName[MAX_NAME_LENGTH + 1];
  StackEntry *m_next;
};

#endif // STACKENTRY_H
