/*
 *     Author : Darren
 * Created on : 06/20 2016
 *
 * Copyright (c) 2016 Darren. All rights reserved.
 */
#include <jni_common/jni_utils.h>
// #include <jni_common/jni_primitives.h>
#include <glog/logging.h>
#include <sstream>
#include <unistd.h>

// Java Integer/Float


JavaVM* g_javaVM = NULL;


namespace jniutils {

char* convertJStrToCStr(JNIEnv* env, jstring lString) {
  const char* lStringTmp;
  char* pstring;

  lStringTmp = env->GetStringUTFChars(lString, NULL);
  if (lStringTmp == NULL)
    return NULL;

  jsize lStringLength = env->GetStringUTFLength(lString);

  pstring = (char*)malloc(sizeof(char) * (lStringLength + 1));
  strcpy(pstring, lStringTmp);

  env->ReleaseStringUTFChars(lString, lStringTmp);

  return pstring;
}

std::string convertJStrToString(JNIEnv* env, jstring lString) {
  const char* lStringTmp;
  std::string str;

  lStringTmp = env->GetStringUTFChars(lString, NULL);
  if (lStringTmp == NULL)
    return NULL;

  str = lStringTmp;

  env->ReleaseStringUTFChars(lString, lStringTmp);

  return str;
}

JNIEnv* vm2env(JavaVM* vm) {
  JNIEnv* env = NULL;
  if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
    DLOG(WARNING) << "vm2env failed";
    env = NULL;
  }
  if (env == NULL) {
    DLOG(WARNING) << "vm2env get NULL env";
  }
  return env;
}
}
